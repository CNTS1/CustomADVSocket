#pragma once

#include "ProtocolHeader.hpp"

class ProtocolValidator
{
 public:
   static bool Validate(const ProtocolHeader& header);
};

// ProtocolValidator.hpp
// Stateless validator that checks a ProtocolHeader for correctness
// before it is processed or accepted.