//
// Created by matvey on 5/7/23.
//

#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H


#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <mutex>

enum PacketType
{
    PT_ClientName,
    PT_ReceiverName,
    PT_ChatMessage,
    PT_MessageWithReceiverName,
    PT_ClientGetNewMessages,
    PT_MessageWithSenderName,
    PT_StartSendClientMessages,
    PT_FinishSendClientMessages,
};

enum ServerAnswers
{
    SA_OK,
    SA_SUCH_NAME_TAKEN,
    SA_ERROR_CLIENT_NAME_EXPECTED,
    SA_NO_SUCH_NAME,
};

enum ClientAnswers
{
    CA_OK,
    CA_ERROR,
};

using SOCKET = int;

class Client
{
public:
    void start();

private:
    void fillAddr();
    void makeConnection();
    void login();
    void sendMessage();
    void getMessages();
    void requestMessages(int interaval);
    void mainLoop();
    void sendPacketType(PacketType packetType);
    PacketType getPacketType();
    void sendAnswer(ClientAnswers answer);
    ServerAnswers getServerAnswer();
    void sendString(const std::string& str);
    std::string getString();
    void recv_s(SOCKET s, char* buf, int len, int flags = 0);
    void send_s(SOCKET s, const char* buf, int len, int flags = 0);

private:
    sockaddr_in addr;
    SOCKET connection;
    std::mutex server_mutex;
    bool is_end = false;
    bool is_authorize = false;
};


#endif //CLIENT_CLIENT_H
