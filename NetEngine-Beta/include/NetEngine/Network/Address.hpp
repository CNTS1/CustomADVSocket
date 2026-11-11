#pragma once

#include <winsock2.h>
#include <string>

class Address
{
public:
   Address();

   Address(const std::string& ip, unsigned short port);

   bool SetAddress(const std::string&ip, unsigned short port);

   const sockaddr* GetSockAddr() const;

   int GetLength() const;

private:
   sockaddr_in m_Address;
};

// Address.hpp
// Wraps a sockaddr_in and exposes helpers to set an IPv4 address/port
// pair and retrieve it as a raw sockaddr for socket calls.