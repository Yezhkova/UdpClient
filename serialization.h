#pragma once
#include <cereal/types/string.hpp>
#include <cereal/archives/binary.hpp>
#define MAXLINE  (1024*1024)

std::string bufToPacket(char buffer[MAXLINE])
{
    uint16_t * packetLengthPtr = reinterpret_cast<uint16_t *> (buffer);
    uint16_t packetLength = * packetLengthPtr;
    std::string packet{buffer+2, packetLength};
    return packet;
}

struct CmdConnectToServer
{
  char command = '1';
  std::string username;
  template<class Archive>

  void serialize(Archive & archive)
  {
    archive(command, username);
  }
};


struct CmdDisconnectFromServer
{
  char command = '0';
  std::string username;
  template<class Archive>

  void serialize(Archive & archive)
  {
    archive(command, username);
  }
};

struct CmdClientinfo
{
    char command;
    std::string username;
    char delimeter = ';';
    std::string address = "";
    std::string port = "";

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(command, username, delimeter, address, delimeter, port);
    }
};



