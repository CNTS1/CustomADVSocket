#pragma once

#include <string>

class Error
{
 public:
    static int GetCode();

    static std::string GetMessage();
};

// Error.hpp
// Static interface for retrieving the last error code and its
// corresponding human-readable message.