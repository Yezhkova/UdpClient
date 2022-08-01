#ifndef CLIENTDATA_H
#define CLIENTDATA_H
#pragma once

#include <cstdint>
#include <netinet/in.h>
#include <string>

class ClientData
{
public:
    ClientData() = default;
    ClientData(std::string address, in_port_t port)
    {
        m_address = address;
        m_port = port;
    }

    const std::string getAddress()
    {
        return m_address;
    }

    const in_port_t getPort()
    {
        return m_port;
    }

private:
    std::string  m_address;
    in_port_t    m_port;
};

#endif // CLIENTDATA_H
