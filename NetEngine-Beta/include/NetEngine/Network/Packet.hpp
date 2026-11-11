#pragma once


#include "../Core/Protocol/PacketType.hpp"

// ===================================================================
// Core Engine - Packet Serialization & Deserialization
// ===================================================================

#include <vector>
#include <string>
#include <cstdint>

class Packet
{
public:
    // -------------------------------------------------------------------
    // Constructors & Buffer Management
    // -------------------------------------------------------------------
    Packet();
     
    void clear();
    void Assign(const void* data, std::size_t size);

    // -------------------------------------------------------------------
    // Buffer Accessors & Attributes
    // -------------------------------------------------------------------
    const std::uint8_t* Data() const;
    std::size_t Size() const;

    // -------------------------------------------------------------------
    // Serialization (Write Interface)
    // -------------------------------------------------------------------
    void WriteBytes(const void* data, std::size_t size);
    void WriteUInt32(std::uint32_t value);
    void WriteInt32(std::int32_t value);
    void WriteString(const std::string& text);

    // -------------------------------------------------------------------
    // Deserialization (Read Interface)
    // -------------------------------------------------------------------
    bool ReadBytes(void* destination, std::size_t size);
    bool ReadUInt32(std::uint32_t& value);
    bool ReadInt32(std::int32_t& value);
    bool ReadString(std::string& text);
    const char* PacketTypeToStirng(PacketType type);

    void SetType(PacketType type);
    PacketType GetType() const;

    // -------------------------------------------------------------------
   // Get Paylaod Size
  // -------------------------------------------------------------------
  std::uint32_t GetPayloadSize() const;

private:
    // -------------------------------------------------------------------
    // Internal Buffer State & Helpers
    // -------------------------------------------------------------------
    std::vector<std::uint8_t> m_buffer;
    std::size_t m_ReadPosition;
    PacketType m_Type = PacketType::Unknown;

    bool CanRead(std::size_t bytes) const;
};