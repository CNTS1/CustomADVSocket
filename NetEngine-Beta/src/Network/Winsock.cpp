/*==============================================================================
   Winsock.cpp
 ------------------------------------------------------------------------------
   Implementation of the Winsock initialization helper.

   Ensures WSAStartup is called exactly once and pairs it with a matching
   WSACleanup on shutdown. The initialization flag is a process-wide static
   and is not thread-safe: Initialize() and Cleanup() must be called from a
   single thread, typically at program start and exit.

   See Winsock.hpp for the full interface description.
 ==============================================================================*/

#include "NetEngine/Network/Winsock.hpp"
#include <iostream>

bool Winsock::m_Initialized = false;

// Initializes Winsock 2.2 once; subsequent calls are no-ops.
bool Winsock::Initialize()
{
    if(m_Initialized)
    return true;

    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(result != 0)
    {
        std::cerr <<"[Winsock] WSAStartup failed with error: " << result << std::endl;
        return false;
    }
    m_Initialized = true;
    return true;
}

// Releases Winsock resources if it was initialized.
void Winsock::Cleanup()
{
    if(!m_Initialized)
    return;

    WSACleanup();
    m_Initialized = false;
}