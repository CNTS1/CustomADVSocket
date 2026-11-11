#pragma once

#include <string>

class NetworkError
{
  public:
    static std::string GetErrorMessage(int errorCode);
};

// NetworkError.h
// Utility class exposing a static helper that maps an integer error code
// to its corresponding human-readable message.