#include "ClientData.h"
#include "IMessenger.h"
#include "serialization.h"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <map>
#include <thread>
#include <QDebug>
#include <unistd.h>
#include "serialization.h"

#define MAXLINE  (1024*1024)
#define LOG(expr) {std::stringstream ss; ss << expr; qDebug() << QString::fromStdString(ss.str());}

class UdpMessenger: public IMessenger
{
public:
    int                                 m_sockfd;
    std::string                         m_username;
    std::string                         m_serverAddr;
    uint16_t                            m_serverPort;
    struct sockaddr_in                  m_sockAddrIn;
    std::map<std::string, ClientData>   m_clientmap;
    std::thread                         m_readingThread;
    MessengerDelegate &                 m_delegate;

public:

    UdpMessenger(MessengerDelegate & delegate) : m_delegate(delegate)
    {

    }

    void sendToOldClients(char * buffer, size_t bytes)
    {
        for(auto& [key, val] : m_clientmap)
        {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = val.getPort();
            //LOG(addr.sin_port<<'\t';
            addr.sin_addr.s_addr = inet_addr(val.getAddress().c_str());
            //LOG(addr.sin_addr.s_addr<<'\n';
            int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            ssize_t n = sendto(sockfd, buffer, bytes,
                               MSG_CONFIRM, (const struct sockaddr *) &addr,
                               sizeof(addr));
            if(n<=0)
            {
                perror("Transmission error: ");
            }
            ::close(sockfd);
        }
    }

    void sendMessageToClient(std::string destName, char * buffer, int bytes)
    {
        if(auto it = m_clientmap.find(destName); it != m_clientmap.end())
        {
            ClientData &destData = it->second;
            LOG("I am writing to "<<destName<<'\t'<<destData.getPort()<<'\t'<<destData.getAddress());
            {
                struct sockaddr_in addr;
                memset(&addr, 0, sizeof(addr));
                addr.sin_family = AF_INET;
                addr.sin_port = destData.getPort();
                //LOG(addr.sin_port<<'\t';
                addr.sin_addr.s_addr = inet_addr(destData.getAddress().c_str());
                //LOG(addr.sin_addr.s_addr<<'\n';
                int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                ssize_t n = sendto(sockfd, buffer, bytes,
                                   MSG_CONFIRM, (const struct sockaddr *) &addr,
                                   sizeof(addr));
                if(n<=0)
                {
                    perror("Transmission error: ");
                }
                ::close(sockfd);
            }
        }

    }

    virtual const std::string & username() const override
    {
        return m_username;
    }

    virtual void connect(const std::string & username, const std::string & address, uint16_t port) override // throws exception
    {
        m_username = username;
        m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        memset(&m_sockAddrIn, 0, sizeof(m_sockAddrIn));
        // Filling server information
        m_sockAddrIn.sin_family = AF_INET; // IPv4
        m_sockAddrIn.sin_addr.s_addr = inet_addr(address.c_str());
        m_serverAddr = address;
        m_sockAddrIn.sin_port = htons(port);
        m_serverPort = port;
        {
            std::stringstream ss;
            cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
            CmdConnectToServer cmd {.username = m_username};
            oarchive(cmd);
            size_t packetLength = ss.str().length();
            char * buffer = new char[packetLength + 2];
            uint16_t * packetLengthPtr = reinterpret_cast<uint16_t *>(buffer);
            * packetLengthPtr = packetLength;
            std::memcpy(buffer + 2, ss.str().c_str(), packetLength);
            sendto(m_sockfd, buffer, packetLength+2,
                   MSG_CONFIRM, (const struct sockaddr *) &m_sockAddrIn,
                   sizeof(m_sockAddrIn));
            delete[] packetLengthPtr;
            LOG(m_username<< "\nHello message sent");
        }
        threadListen();
    }

    void threadListen()
    {
        m_readingThread = std::thread([&, this]{
            while(true)
            {
                try
                {
                    char command,delim1, delim2 ;
                    std::string username, addressOrMessage, port ;

                    {
                        char buffer[MAXLINE];
                        ssize_t n;
                        uint len;

                        n = recvfrom(m_sockfd, buffer, MAXLINE,
                                     MSG_WAITALL, ( struct sockaddr *) &m_sockAddrIn,
                                     &len);
                        LOG(n<<" bytes received");
                        std::string packet = bufToPacket(buffer);
                        std::istringstream sBuf(packet, std::ios::binary);
                        cereal::BinaryInputArchive iarchive(sBuf);
                        iarchive(command, username, delim1, addressOrMessage, delim2, port);
                    }
                    LOG(command <<", "<<username<<", "<<addressOrMessage<<", "<<port);
                    switch (command) {
                    case 'u':
                        LOG("now online: "<<username<<", "<<addressOrMessage<<", "<<port);
                        m_clientmap[username] = {addressOrMessage, static_cast<in_port_t>(std::stoi(port))};
                        LOG("I will evoke m_delegate");
                        m_delegate.onUserConnected(username);
                      break;
                    case 'd':
                    {
                        m_clientmap.erase(username);
                        m_delegate.onUserDisconnected(username);
                        break;
                    }
                    case 'm':
                        m_delegate.onMessageReceived(username, addressOrMessage);
                        break;
                    default:
                        std::cerr<<"Invalid command from server or another client :" << command<< "(int:) "<<int(command)<<'\n';
                        break;
                    }
                }
                catch(std::exception& e)
                {
                    LOG("client readingUserList thread error: "<<e.what());
                }
            }
        });

    }
    virtual void disconnect() override
    {
        {
            std::stringstream ss;
            cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
            CmdClientinfo cmd {.command = 'd', .username = m_username };
            oarchive(cmd);
            size_t packetLength = ss.str().length();
            char * buffer = new char[packetLength + 2];
            uint16_t * packetLengthPtr = reinterpret_cast<uint16_t *>(buffer);
            * packetLengthPtr = packetLength;
            std::memcpy(buffer + 2, ss.str().c_str(), packetLength);

            sendToOldClients(buffer, packetLength+2);
        }
        {
            std::stringstream ss;
            cereal::BinaryOutputArchive oarchive(ss); // Create an output archive
            CmdDisconnectFromServer cmd {.username = m_username };
            oarchive(cmd);
            size_t packetLength = ss.str().length();
            char * buffer = new char[packetLength + 2];
            uint16_t * packetLengthPtr = reinterpret_cast<uint16_t *>(buffer);
            * packetLengthPtr = packetLength;
            std::memcpy(buffer + 2, ss.str().c_str(), packetLength);

            struct sockaddr_in server;
            memset(&server, 0, sizeof(server));
            // Filling server information
            server.sin_family = AF_INET; // IPv4
            server.sin_addr.s_addr = inet_addr(m_serverAddr.c_str());
            server.sin_port = htons(m_serverPort);
            ssize_t n = sendto(m_sockfd, buffer, packetLength+2,
                               MSG_CONFIRM, (const struct sockaddr *) &server,
                               sizeof(server));
            if(n<=0){
                perror("Disconnect from Server error: ");
            }
            LOG("Disconnection message sent to "<<"???"<<" ("<<ntohs(server.sin_port)<<", "<<server.sin_addr.s_addr<<'\n');
            ::close(m_sockfd);
        }
    }

    virtual void sendMessage(std::string destName, std::string messText) override
    {
        std::stringstream ss;
        cereal::BinaryOutputArchive oarchive(ss); // Create an output archive

        CmdClientinfo cmd {.command='m', .username = m_username, .address = messText};
        oarchive(cmd);
        size_t packetLength = ss.str().length();
        char * buffer = new char[packetLength + 2];
        uint16_t * packetLengthPtr = reinterpret_cast<uint16_t *>(buffer);
        * packetLengthPtr = packetLength;
        std::memcpy(buffer + 2, ss.str().c_str(), packetLength);

        m_delegate.onUserConnected(messText);
//        if(destName == "All")
//        {
//            sendToOldClients(buffer, packetLength+2);
//        }
//        else
//        {
//            sendMessageToClient(destName, buffer, packetLength+2);
//        }

    }

   //members of Mainwindow

};

std::shared_ptr<IMessenger> createUdpMessenger(MessengerDelegate & mDel)
{
    return std::make_shared<UdpMessenger> (mDel);
}

