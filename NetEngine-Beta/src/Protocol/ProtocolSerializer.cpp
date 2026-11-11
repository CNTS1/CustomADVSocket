/*==============================================================================
   ProtocolSerializer.cpp
 ------------------------------------------------------------------------------
   Implementation of the ProtocolSerializer wire format.

   Converts a ProtocolHeader into a fixed 12-byte network-order buffer and
   back. All multi-byte fields are stored in network byte order.

   Deserialize() performs only a size check (>= 12 bytes). Semantic
   validation (Magic, Version, Type, PayloadSize bounds) is intentionally
   left to the caller — see Socket::ReceivePacket and ProtocolValidator.

   See ProtocolSerializer.hpp for the full interface description.
 ==============================================================================*/

#include "NetEngine/Core/Protocol/ProtocolSerializer.hpp"
#include "NetEngine/Core/Utils/Endian.hpp"

#include <cstring>

// Serializes a ProtocolHeader into a 12-byte network-order buffer.
std::vector<std::uint8_t> ProtocolSerializer::Serialize(const ProtocolHeader& header)
{
    std::vector<std::uint8_t> buffer;

    buffer.resize(12);

    std::uint8_t* data = buffer.data();

    const std::uint32_t magic = Endian::HostToNetwork32(header.Magic);

    const std::uint16_t version = Endian::HostToNetwork16(header.Version);

    const std::uint16_t type = Endian::HostToNetwork16(static_cast<std::uint16_t>(header.Type));

    const std::uint32_t payloadSize = Endian::HostToNetwork32(header.PayloadSize);


    std::memcpy(data, &magic, sizeof(magic));

    std::memcpy(data + 4, &version, sizeof(version));

    std::memcpy(data + 6, &type, sizeof(type));

    std::memcpy(data + 8, &payloadSize, sizeof(payloadSize));

    return buffer;
}


// Deserializes the first 12 bytes into header.
// NOTE: This function performs only a size check (>= 12 bytes).
// Semantic validation (Magic, Version, Type, PayloadSize bounds) must be
// performed by the caller — see Socket::ReceivePacket and ProtocolValidator.
bool ProtocolSerializer::Deserialize(const std::vector<std::uint8_t>& data, ProtocolHeader& header)
{
    if (data.size() < 12)
    {
        return false;
    }

    const std::uint8_t* bytes = data.data();

    std::uint32_t magic = 0;
    std::uint16_t version = 0;
    std::uint16_t type = 0;
    std::uint32_t payloadSize = 0;

    std::memcpy(&magic, bytes, sizeof(magic));

    std::memcpy(&version, bytes + 4, sizeof(version));

    std::memcpy(&type, bytes + 6, sizeof(type));

    std::memcpy(&payloadSize, bytes + 8, sizeof(payloadSize));

    header.Magic = Endian::NetworkToHost32(magic);

    header.Version = Endian::NetworkToHost16(version);

    header.Type = static_cast<PacketType>(Endian::NetworkToHost16(type));

    header.PayloadSize = Endian::NetworkToHost32(payloadSize);

    return true;
}