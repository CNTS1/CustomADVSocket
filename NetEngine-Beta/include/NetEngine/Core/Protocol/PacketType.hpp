#pragma once

#include <cstdint>

enum class PacketType : std::uint16_t
{
   Unknown = 0,

   Handshake = 1,

   Ping = 2,

   Pong = 3,

   Command = 4,

   Response = 5,
   
   Beacon = 6,

   CommandReq = 7,

   CommandResp = 8,

   Disconnect = 9
};

// PacketType.h
// Strongly-typed enum representing the different kinds of packets
// exchanged over the network protocol.