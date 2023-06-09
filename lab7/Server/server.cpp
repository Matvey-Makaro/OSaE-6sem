//
// Created by matvey on 5/7/23.
//

#include "server.h"

#include <iostream>
#include <exception>
#include <memory>
#include <utility>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

void Server::start()
{
    fillAddr();
    sizeofaddr = sizeof(addr);
    createListener();

    SOCKET newConnection;
    while (true)
    {
        newConnection = accept(sListener, reinterpret_cast<sockaddr*>(&addr),
                               reinterpret_cast<socklen_t *>(&sizeofaddr));
        if (newConnection == -1)
        {
            cerr << "Error accept new connection! " << errno << endl;
            continue;
        }

        threads.emplace_back([this, newConnection]() {
            clientHandler(newConnection);
        });

    }

#if 1
    cout << "Client connected!\n";
    char msg[256] = "Hello, client!";
    send(newConnection, msg, sizeof(msg), 0);

    for (auto& t : threads)
        t.join();
#endif
}

Server::~Server()
{
    for (auto& [name, socket] : nameToSocket)
        close(socket);
    close(sListener);

    cout << "Server closed." << endl;
}

void Server::fillAddr()
{
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8888);
    addr.sin_family = AF_INET;
}

void Server::createListener()
{
    sListener = socket(AF_INET, SOCK_STREAM, 0);
    if (sListener == -1)
        throw runtime_error("Socket creation failed! " + std::to_string(errno));

    bind(sListener, reinterpret_cast<sockaddr*>(&addr), sizeofaddr);
    listen(sListener, SOMAXCONN);
}

string Server::clientNameHandler(SOCKET currConnection)
{
    PacketType packetType = getPacketType(currConnection);
    if (packetType != PT_ClientName)
    {
        sendAnswer(currConnection, SA_ERROR_CLIENT_NAME_EXPECTED);
        throw runtime_error("Client name expected for SOCKET: " + to_string(currConnection));
    }

    while (true)
    {
        std::string name = getStringFromClient(currConnection);

        {
            lock_guard<mutex> g(nameToSocketMutex);
            if (nameToSocket.count(name) == 0)
            {
                nameToSocket[name] = currConnection;
                {
                    lock_guard g(receiverNameToMutexMutex);
                    receiverNameToMutex[name];
                }
                sendAnswer(currConnection, SA_OK);
                cout << name << " connected.\n";
                return name;
            }
        }

        sendAnswer(currConnection, SA_SUCH_NAME_TAKEN);
    }
}

string Server::receiverNameHandler(SOCKET currConnection)
{
    string name = getStringFromClient(currConnection);
    {
        lock_guard<mutex> g(nameToSocketMutex);
        if (nameToSocket.count(name) != 0)
        {
            sendAnswer(currConnection, SA_OK);
            return name;
        }
    }
    sendAnswer(currConnection, SA_NO_SUCH_NAME);
    return "";
}

void Server::chatMessageHandler(SOCKET currConnection, string sender, const string& receiver)
{
    string message = getStringFromClient(currConnection);

    receiverToMessagesMutex.lock();
    auto& messages = receiverToMessages[receiver];
    receiverToMessagesMutex.unlock();

    {
        lock_guard g(receiverNameToMutex[receiver]);
        messages.push_back({ move(sender), move(message) });
    }
}

void Server::clientGetNewMessagesHandler(SOCKET currConnection, const string& currClientName)
{
    sendPacketType(currConnection, PT_StartSendClientMessages);
    receiverToMessagesMutex.lock();
    auto& messages = receiverToMessages[currClientName];
    receiverToMessagesMutex.unlock();

    {
        lock_guard g(receiverNameToMutex[currClientName]);
        for (size_t i = 0; i < messages.size(); i++)
        {
            sendPacketType(currConnection, PT_MessageWithSenderName);

            sendStringToClient(currConnection, messages[i].sender);
            sendStringToClient(currConnection, messages[i].body);
        }
        messages.clear();
    }

    sendPacketType(currConnection, PT_FinishSendClientMessages);
}

void Server::clientHandler(SOCKET currConnection)
{
    std::cout << "Client handler!" << std::endl;
    std::string currClientName;
    try
    {
        currClientName = clientNameHandler(currConnection);
        string receiverName;
        while (true)
        {
            PacketType packetType = getPacketType(currConnection);
            switch (packetType)
            {
                case PT_ReceiverName:
                    receiverName = receiverNameHandler(currConnection);
                    break;
                case PT_ChatMessage:
                    chatMessageHandler(currConnection, currClientName, receiverName);
                    break;
                case PT_ClientGetNewMessages:
                    clientGetNewMessagesHandler(currConnection, currClientName);
                    break;
                default:
                    cerr << "Unknown packet type" << endl;
                    break;
            }
        }
    }
    catch (const std::exception& ex)
    {
        cerr << ex.what() << endl;
        close(currConnection);
        if(!currClientName.empty())
        {
            {
                lock_guard guard(nameToSocketMutex);
                nameToSocket.erase(currClientName);
            }

            {
                lock_guard guard(receiverToMessagesMutex);
                if (receiverToMessages.count(currClientName) != 0)
                    receiverToMessages.erase(currClientName);
            }

            {
                lock_guard guard(receiverNameToMutexMutex);
                if (receiverNameToMutex.count(currClientName) != 0)
                    receiverNameToMutex.erase(currClientName);
            }
        }

    }
}

void Server::sendAnswer(SOCKET socket, ServerAnswers answer)
{
    send_s(socket, reinterpret_cast<const char*>(&answer), sizeof(answer));
}

ClientAnswers Server::getClientAnswer(SOCKET socket)
{
    ClientAnswers clientAnswer;
    recv_s(socket, reinterpret_cast<char*>(&clientAnswer), sizeof(clientAnswer));
    return clientAnswer;
}

PacketType Server::getPacketType(SOCKET socket)
{
    PacketType packetType;
    recv_s(socket, reinterpret_cast<char*>(&packetType), sizeof(packetType));
    return packetType;
}

void Server::sendPacketType(SOCKET socket, PacketType packetType)
{
    send_s(socket, reinterpret_cast<const char*>(&packetType), sizeof(packetType));
}

std::string Server::getStringFromClient(SOCKET socket)
{
    constexpr int bufferSize = 1024;
    char buffer[bufferSize];
    int strSize = 0;
    recv_s(socket, reinterpret_cast<char*>(&strSize), sizeof(strSize));
    if (strSize <= 0)
        throw runtime_error("Size of string <= 0.");

    unique_ptr<char> dynamic_buffer;
    char* result_buffer;
    if (strSize >= bufferSize)
    {
        dynamic_buffer = unique_ptr<char>(new char[strSize + 1]);
        result_buffer = dynamic_buffer.get();
    }
    else result_buffer = buffer;

    result_buffer[strSize] = '\0';

    recv_s(socket, result_buffer, strSize);

    return string(result_buffer);
}

void Server::sendStringToClient(SOCKET socket, const std::string str)
{
    int size = str.size();
    send_s(socket, reinterpret_cast<const char*>(&size), sizeof(size));
    send_s(socket, str.c_str(), size);
}

void Server::recv_s(SOCKET s, char* buf, int len, int flags)
{
    auto recvResult = recv(s, buf, len, flags);
    if (recvResult == -1)
        throw runtime_error("Recv error: " + to_string(errno));
}

void Server::send_s(SOCKET s, const char* buf, int len, int flags)
{
    auto sendResult = send(s, buf, len, flags);
    if (sendResult == -1)
        throw runtime_error("Send error: " + to_string(errno));
}