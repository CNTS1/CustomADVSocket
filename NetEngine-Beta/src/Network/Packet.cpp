// ===================================================================
// Engine Libraries - Packet Implementation
// ===================================================================

#include "NetEngine/Network/Packet.hpp"
#include "NetEngine/Core/Protocol/ProtocolSerializer.hpp"

#include <cstring>
#include <iostream>
#include <vector>
#include <string>

// -------------------------------------------------------------------
// Constructor & Buffer Lifecycle
// -------------------------------------------------------------------

Packet::Packet() : m_ReadPosition(0) // default constructor, initializes read cursor to zero
{

}

void Packet::clear() // resets buffer contents and read cursor
{
    m_buffer.clear();
    m_ReadPosition = 0;
}

void Packet::Assign(const void* data, std::size_t size) // copies raw bytes into the buffer and resets read cursor
{
    clear();

    if (size == 0 || data == nullptr)
    {
        return;
    }

    const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
    m_buffer.assign(bytes, bytes + size);

    m_ReadPosition = 0;
}

// -------------------------------------------------------------------
// Buffer Accessors & Attributes
// -------------------------------------------------------------------

const std::uint8_t* Packet::Data() const // returns pointer to the raw buffer bytes
{
    return m_buffer.data();
}

std::size_t Packet::Size() const // returns the total size of the buffer in bytes
{
    return m_buffer.size();
}

// -------------------------------------------------------------------
// Serialization (Write Interface)
// -------------------------------------------------------------------

void Packet::WriteBytes(const void* data, std::size_t size) // appends raw bytes to the end of the buffer
{
    if (size == 0 || data == nullptr)
    {
        return;
    }

    const auto* bytes = static_cast<const std::uint8_t*>(data);
    m_buffer.insert(m_buffer.end(), bytes, bytes + size);
}

void Packet::WriteUInt32(std::uint32_t value) // appends a 32-bit unsigned integer
{
    WriteBytes(&value, sizeof(value));
}

void Packet::WriteInt32(std::int32_t value) // appends a 32-bit signed integer
{
    WriteBytes(&value, sizeof(value));
}

void Packet::WriteString(const std::string& text) // appends a length-prefixed string (uint32 size + bytes)
{
    if (text.size() > static_cast<std::size_t>(0xFFFFFFFFu))
    {
        return;
    }

    WriteUInt32(static_cast<std::uint32_t>(text.size()));
    WriteBytes(text.data(), text.size());
}

// -------------------------------------------------------------------
// Deserialization & Bounds Checking (Read Interface)
// -------------------------------------------------------------------

bool Packet::CanRead(std::size_t bytes) const // checks if enough unread bytes remain in the buffer
{
    if (bytes > m_buffer.size())
    {
        return false;
    }

    return m_ReadPosition <= (m_buffer.size() - bytes);
}

bool Packet::ReadBytes(void* destination, std::size_t size) // copies bytes from read cursor into destination
{
    if (size == 0)
    {
        return true;
    }

    if (destination == nullptr || !CanRead(size))
    {
        return false;
    }

    std::memcpy(destination, m_buffer.data() + m_ReadPosition, size);
    m_ReadPosition += size;
    return true;
}

bool Packet::ReadUInt32(std::uint32_t& value) // reads a 32-bit unsigned integer into value
{
    return ReadBytes(&value, sizeof(value));
}

bool Packet::ReadInt32(std::int32_t& value) // reads a 32-bit signed integer into value
{
    return ReadBytes(&value, sizeof(value));
}

bool Packet::ReadString(std::string& text) // reads a length-prefixed string (uint32 size + bytes)
{
    std::uint32_t length = 0;
    if (!ReadUInt32(length))
    {
        return false;
    }

    if (!CanRead(length))
    {
        return false;
    }

    text.assign(reinterpret_cast<const char*>(m_buffer.data() + m_ReadPosition), length);
    m_ReadPosition += length;

    return true;
}


void Packet::SetType(PacketType type) // sets the packet's type identifier
{
    m_Type = type;
}

PacketType Packet::GetType() const // returns the packet's type identifier
{
    return m_Type;
}

const char* Packet::PacketTypeToStirng(PacketType type) // maps a PacketType to its readable name
{
    switch (type)
    {
        case PacketType::Ping:
            return "Ping";

        case PacketType::Pong:
            return "Pong";

        case PacketType::Command:
            return "Command";

        default:
            return "Unknown";
    }
}

std::uint32_t Packet::GetPayloadSize() const // returns current buffer size as payload size
{
    return static_cast<std::uint32_t>(m_buffer.size());
}