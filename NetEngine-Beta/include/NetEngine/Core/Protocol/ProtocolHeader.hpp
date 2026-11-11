#pragma once

#include <cstdint>

#include "PacketType.hpp"

struct ProtocolHeader
{
  std::uint32_t Magic = 0x4153434B;
  std::uint16_t Version = 1;
  PacketType Type = PacketType::Unknown;
  std::uint32_t PayloadSize = 0;
};

// ProtocolHeader.hpp
// Fixed-size header prepended to every protocol packet, carrying the
// magic signature, protocol version, packet type, and payload length.