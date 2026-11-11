#pragma once 

#include <winsock2.h>

class Winsock
{
  public:
    static bool Initialize();
    
    static void Cleanup();

  private:
    static bool m_Initialized;
};

// Winsock.hpp
// Manages the lifetime of the Winsock library, providing static
// initialization and cleanup entry points.