//
// Created by matvey on 5/7/23.
//

#include "client.h"
#include <iostream>
#include <exception>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

void Client::start()
{
    fillAddr();
    makeConnection();
    cout << "Connected!\n";

    constexpr int interval = 2000000;
    thread requestMessagesThread([this, interval]() {
        requestMessages(interval);
    });
    requestMessagesThread.detach();

    mainLoop();

    is_end = true;
}

void Client::fillAddr()
{
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8888);
    addr.sin_family = AF_INET;
}

void Client::makeConnection()
{
    connection = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(connection, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1)
        throw runtime_error("Error connect! Code: " + std::to_string(errno));
}

void Client::login()
{
    lock_guard g(server_mutex);
    sendPacketType(PT_ClientName);
    while (true)
    {
        cout << "Enter your name: ";
        string name;
        getline(cin, name);
        sendString(name);

        ServerAnswers answer = getServerAnswer();
        if (answer == SA_OK)
        {
            cout << "Login completed successfully." << endl;
            is_authorize = true;
            return;
        }
        else if (answer == SA_SUCH_NAME_TAKEN)
            cout << "This name is already taken." << endl;
        else if (answer == SA_ERROR_CLIENT_NAME_EXPECTED)
            throw runtime_error("Client name expected!");
        else throw runtime_error("Unknown error!");
    }
}

void Client::sendMessage()
{
    while (true)
    {
        cout << "Enter receiver: ";
        string receiver;
        getline(cin, receiver);

        unique_lock lock(server_mutex);
        sendPacketType(PT_ReceiverName);
        cout << "Receiver " << receiver << endl;

        sendString(receiver);

        auto serverAnswer = getServerAnswer();
        lock.unlock();

        if (serverAnswer == SA_OK)
            break;
        else if (serverAnswer != SA_NO_SUCH_NAME)
            throw runtime_error("Unknown sever answer");

        cout << "No such user. Try again.\n";
    }


    cout << "Enter message:\n";
    string message;
    getline(cin, message);
    if(!message.empty())
    {
        lock_guard g(server_mutex);
        sendPacketType(PT_ChatMessage);
        sendString(message);
    }

}

void Client::getMessages()
{
    lock_guard g(server_mutex);
    bool is_new_messages_print = true;
    sendPacketType(PT_ClientGetNewMessages);
    auto packet_type = getPacketType();
    if (packet_type != PT_StartSendClientMessages)
        throw std::runtime_error("Unexpected packet type from server.");

    packet_type = getPacketType();
    while (packet_type == PT_MessageWithSenderName)
    {

        string sender = getString();
        string messageBody = getString();

        if (is_new_messages_print)
        {
            cout << "\nNew messages:" << endl;
            is_new_messages_print = false;
        }

        cout << "Sender: " << sender << '\n';
        cout << "Body: " << messageBody << "\n" <<  endl;


        packet_type = getPacketType();
    }

    if (packet_type != PT_FinishSendClientMessages)
        throw std::runtime_error("Unexpected packet type from server.");
}

void Client::requestMessages(int interaval)
{
    while (!is_authorize)
        usleep(100000);

    while (!is_end)
    {
        getMessages();
        usleep(interaval);
    }
}

void Client::mainLoop()
{
    login();

    while (true)
    {
        sendMessage();
        usleep(10000);
    }
}

void Client::sendPacketType(PacketType packetType)
{
    send_s(connection, reinterpret_cast<const char*>(&packetType), sizeof(packetType));
}

PacketType Client::getPacketType()
{
    PacketType packetType;
    recv_s(connection, reinterpret_cast<char*>(&packetType), sizeof(packetType));
    return packetType;
}

void Client::sendAnswer(ClientAnswers answer)
{
    send_s(connection, reinterpret_cast<const char*>(&answer), sizeof(answer));
}

ServerAnswers Client::getServerAnswer()
{
    ServerAnswers answer;
    recv_s(connection, reinterpret_cast<char*>(&answer), sizeof(answer));
    return answer;
}

void Client::sendString(const string& str)
{
    int size = str.size();
    send_s(connection, reinterpret_cast<const char*>(&size), sizeof(size));
    send_s(connection, str.c_str(), size);
}

string Client::getString()
{
    constexpr int bufferSize = 1024;
    char buffer[bufferSize];
    int strSize = 0;
    recv_s(connection, reinterpret_cast<char*>(&strSize), sizeof(strSize));
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

    recv_s(connection, result_buffer, strSize);

    return result_buffer;
}

void Client::recv_s(SOCKET s, char* buf, int len, int flags)
{
    auto recvResult = recv(s, buf, len, flags);
    if (recvResult == -1)
        throw runtime_error("Recv error: " + to_string(errno));
}

void Client::send_s(SOCKET s, const char* buf, int len, int flags)
{
    auto sendResult = send(s, buf, len, flags);
    if (sendResult == -1)
        throw runtime_error("Send error: " + to_string(errno));
}