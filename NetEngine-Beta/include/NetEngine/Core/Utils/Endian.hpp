#pragma once

#include <cstdint>

class Endian
{
  public:
  
    static std::uint16_t HostToNetwork16(std::uint16_t value);

    static std::uint32_t HostToNetwork32(std::uint32_t value);

    static std::uint16_t NetworkToHost16(std::uint16_t value);

    static std::uint32_t NetworkToHost32(std::uint32_t value);
};

// Endian.hpp
// Utility class providing static helpers to convert 16-bit and 32-bit
// integers between host byte order and network byte order.