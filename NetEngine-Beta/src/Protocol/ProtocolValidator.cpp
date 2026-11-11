/*==============================================================================
   ProtocolValidator.cpp
 ------------------------------------------------------------------------------
   Implementation of the ProtocolValidator header validation rules.

   Performs semantic checks on a ProtocolHeader after deserialization:
   magic, version, packet type, and payload size bounds. Returns true only
   when every rule passes.

   See ProtocolValidator.hpp for the full interface description.
 ==============================================================================*/

#include "NetEngine/Core/Protocol/ProtocolValidator.hpp"

// Validates magic, version, type, and payload size of a ProtocolHeader.
bool ProtocolValidator::Validate(const ProtocolHeader& header)
{
    if(header.Magic != 0x4153434B)
    {
        return false;
    }

    if(header.Version != 1)
    {
        return false;
    }

    if(header.Type == PacketType::Unknown)
    {
        return false;
    }

    if(header.PayloadSize > 1024 * 1024)
    {
        return false;
    }

    return true;
}