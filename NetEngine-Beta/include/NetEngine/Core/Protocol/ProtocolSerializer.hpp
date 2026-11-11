#pragma once

#include <vector>
#include <cstdint>

#include "ProtocolHeader.hpp"

class ProtocolSerializer
{
 public:

    static std::vector<std::uint8_t>Serialize(const ProtocolHeader& header);
     
    static bool Deserialize(const std::vector<std::uint8_t>& data, ProtocolHeader& header);
};

// ProtocolSerializer.hpp
// Stateless serializer that converts a ProtocolHeader to and from a raw
// byte buffer for transmission over the wire.