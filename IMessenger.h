#pragma once
#include <memory>
#include <string>

class MessengerDelegate // mainWindow inherits MessengerDelegate
{
public:
    virtual void onUserConnected(std::string username) = 0;
    virtual void onUserDisconnected(std::string username) = 0;
    virtual void onMessageReceived(std::string username, std::string messText) = 0;

};

class IMessenger: public std::enable_shared_from_this<IMessenger> // UdpMessenger, Libtorrent inherits IMessenger
{
public:
//    virtual void setDelegate(MessengerDelegate *) = 0;
    virtual void  connect(const std::string & username, const std::string & address, uint16_t port) = 0; // throws exception
    virtual void  disconnect() = 0;
    virtual void sendMessage(std::string username, std::string messText) = 0;
    virtual const std::string & username() const = 0;
};


class Msg: public IMessenger
{
    std::string m_username = "user";
    MessengerDelegate & m_delegate;
public:
//    virtual void setDelegate(MessengerDelegate *) = 0;
    Msg(MessengerDelegate & delegate): m_delegate(delegate)
    {

    }
    virtual void  connect(const std::string & username, const std::string & address, uint16_t port) override
    {
        m_username = username;
        m_delegate.onUserConnected(username);
    }
    virtual void  disconnect() override
    {

    }
    virtual void sendMessage(std::string username, std::string messText) override
    {
        m_delegate.onUserConnected(messText);

    }
    virtual const std::string & username() const override
    {
        return m_username;
    }
};


std::shared_ptr<IMessenger> createUdpMessenger(MessengerDelegate & mDel);
