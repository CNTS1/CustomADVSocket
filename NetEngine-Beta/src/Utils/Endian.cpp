/*==============================================================================
   Endian.cpp
 ------------------------------------------------------------------------------
   Implementation of the Endian conversion helpers.

   Wraps the Winsock byte-order conversion routines so that higher layers
   can convert between host and network byte order without pulling in
   <winsock2.h> directly.

   See Endian.hpp for the full interface description.
 ==============================================================================*/

#include "NetEngine/Core/Utils/Endian.hpp"
#include <winsock2.h>

// Converts a 16-bit value from host to network byte order.
std::uint16_t Endian::HostToNetwork16(std::uint16_t value)
{
  return htons(value);
}

// Converts a 32-bit value from host to network byte order.
std::uint32_t Endian::HostToNetwork32(std::uint32_t value)
{
  return htonl(value);
}

// Converts a 16-bit value from network to host byte order.
std::uint16_t Endian::NetworkToHost16(std::uint16_t value)
{
    return ntohs(value);
}

// Converts a 32-bit value from network to host byte order.
std::uint32_t Endian::NetworkToHost32(std::uint32_t value)
{
    return ntohl(value);
}