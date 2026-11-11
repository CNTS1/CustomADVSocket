/*==============================================================================
   Address.cpp
 ------------------------------------------------------------------------------
   Implementation of the Address wrapper around sockaddr_in.

   Provides a small, IPv4-only helper that converts a textual IP and a port
   into a sockaddr_in suitable for bind/connect/accept. On failure the
   internal state is reset to a well-defined zero address so callers can
   reliably detect an invalid Address.

   See Address.hpp for the full interface description.
 ==============================================================================*/

// ===================================================================
// Engine Libraries - Address Implementation
// ===================================================================

#include "NetEngine/Network/Address.hpp"

#include <cstring>
#include <ws2tcpip.h>

// Default constructor: zero-initializes the sockaddr_in and sets family to AF_INET.
Address::Address()
{
    std::memset(&m_Address, 0, sizeof(m_Address));
    m_Address.sin_family = AF_INET;
}

// Constructs an address from an IP string and a port.
Address::Address(const std::string& ip, unsigned short port)
{
    std::memset(&m_Address, 0, sizeof(m_Address));
    m_Address.sin_family = AF_INET;
    SetAddress(ip, port);
}

// Fills sockaddr_in from ip/port and returns success.
bool Address::SetAddress(const std::string& ip, unsigned short port)
{
    std::memset(&m_Address, 0, sizeof(m_Address));
    m_Address.sin_family = AF_INET;
    m_Address.sin_port = htons(port);

    int result = inet_pton(AF_INET, ip.c_str(), &m_Address.sin_addr);

    if (result != 1)
    {
        // Reset to a well-defined invalid state on failure.
        std::memset(&m_Address, 0, sizeof(m_Address));
        m_Address.sin_family = AF_INET;
        return false;
    }

    return true;
}

// Returns the raw sockaddr pointer for socket calls.
const sockaddr* Address::GetSockAddr() const
{
    return reinterpret_cast<const sockaddr*>(&m_Address);
}

// Returns the size of the underlying sockaddr_in.
int Address::GetLength() const
{
    return sizeof(m_Address);
}