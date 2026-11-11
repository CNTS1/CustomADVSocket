#pragma once

// ===================================================================
// Core Engine - Socket Wrapper Interface
// ===================================================================

#include <winsock2.h>
#include <mutex>
#include <atomic>
#include <functional>
#include <vector>
#include <thread>
#include <string>
#include <cstdint>
#include "Packet.hpp"
#include "Address.hpp"

// -------------------------------------------------------------------
// Socket Enumerations & States
// -------------------------------------------------------------------

enum class SocketState
{
    Closed,
    Created,
    Connected
};


// -------------------------------------------------------------------
// Core Socket Class Definition
// -------------------------------------------------------------------

class Socket
{
public:
    // -------------------------------------------------------------------
    // Lifecycle Management & Initialization
    // -------------------------------------------------------------------
    Socket();
    ~Socket();
    Socket(const Socket&) = delete;
    Socket& operator = (const Socket&) = delete;

    Socket(Socket&& other) noexcept;
    Socket& operator = (Socket&& other) noexcept;


    bool Create(int af = AF_INET, int type = SOCK_STREAM, int protocol = IPPROTO_TCP);
    void Close();
    bool isValid() const;
    bool IsConnected() const;
    

    // -------------------------------------------------------------------
    // Client Connection & Low-Level I/O Operations
    // -------------------------------------------------------------------
    bool Connect(const Address& address);
    int Send(const void* buffer, int length);
    int Receive(void* buffer, int length);

    // -------------------------------------------------------------------
    // Server Operations (Binding, Listening & Connection Acceptance)
    // -------------------------------------------------------------------
    bool Bind(const Address& address);
    bool Listen(int backlog = 5);
    Socket Accept();
    void SetReceiveTimeout(int milliseconds);
    int GetReceiveTimeout() const;

    // -------------------------------------------------------------------
    // Stream Buffer Processing & Packet Serialization
    // -------------------------------------------------------------------
    bool ReceiveExact(void* data, int size);
    bool SendPacket(const Packet& packet);
    bool ReceivePacket(Packet& packet);

    static constexpr std::uint32_t MAX_PAYLOAD_SIZE = 64 * 1024;

    // -------------------------------------------------------------------
    // Error Handling & Querying
    // -------------------------------------------------------------------
    int GetLastError() const;
    void ClearLastError();

    enum class ReceiveResult
    {
    Success,
    ConnectionClosed,
    InvalidHeader,
    PayloadTooLarge,
    Timeout,
    ReceiveFailed
    };
    ReceiveResult GetLastReceiveResult() const;

    // -------------------------------------------------------------------
    // High-Level Protocol Handlers
    // -------------------------------------------------------------------
    bool ProcessPingPong(const Packet& receivedPacket);

    struct ClientSession
    {
      uint32_t SessionID;
      SOCKET SocketHandle;
      std::string IPAddress;
      uint16_t port;
      uint64_t LastBeaconTime;
      bool IsActive;
    };

    std::vector<ClientSession> GetActiveSessions();
    bool SendCommandToSession(uint32_t sessionID, const Packet& commandPacket);
    void CheckSessionTimeouts(uint64_t timeoutSeconds = 15);


    using PacketCallback = std::function<void(Socket& clientSocket, const Packet& packet)>;

    bool StartServer(const std::string& ip, uint16_t port);
    void StopServer();
    void ListenForClients(PacketCallback onPacketReceived = nullptr);

private:
    // -------------------------------------------------------------------
    // Internal State & Private Helpers
    // -------------------------------------------------------------------
    SOCKET m_Handle;
    SocketState m_State;
    int m_LastError = 0;
    ReceiveResult m_LastReceiveResult;
    bool m_LastReceiveClosed;
    int m_ReceiveTimeout;
    bool m_LastReceiveTimeout;

    // Guards m_Handle, m_State, m_LastError, m_LastReceiveResult,
    // m_LastReceiveClosed, m_LastReceiveTimeout, m_ReceiveTimeout.
    mutable std::mutex m_StateMutex;

    void SetLastError(int error);
    bool IsValidPacketType(PacketType type) const;

    std::vector<ClientSession> m_activeSessions;
    std::mutex m_sessionsMutex;
    uint32_t m_nextSessionId = 1000;

    std::atomic<bool> m_isRunning{false};
    std::vector<std::thread> m_clientThreads;
    std::mutex m_clientMutex;

    void HandleClientThread(SOCKET clientSocketHandle, PacketCallback onPacketReceived);
    
    std::thread m_timeoutThread;
    void TimeoutMonitorLoop(uint64_t intervalSeconds);
};