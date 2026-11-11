/*==============================================================================
   Socket.cpp
 ------------------------------------------------------------------------------
   Implementation of the Socket transport layer.

   Provides TCP client/server primitives, framed packet serialization, and
   a small multi-client session manager. Encryption is intentionally out
   of scope: callers are expected to wrap Send/Receive with a higher-level
   crypto layer before exposing payloads on the wire.

   This file fixes the critical concurrency and lifetime issues present in
   the previous revision while preserving the original public behavior.

   See the Socket header for the full interface description.
 ==============================================================================*/

// ===================================================================
// Engine Libraries - Socket Infrastructure
// ===================================================================

#include <NetEngine/Network/Socket.hpp>
#include <NetEngine/Network/Packet.hpp>
#include <NetEngine/Core/Error/NetworkError.hpp>
#include <NetEngine/Core/Protocol/ProtocolHeader.hpp>
#include <NetEngine/Core/Protocol/ProtocolValidator.hpp>
#include <NetEngine/Core/Protocol/ProtocolSerializer.hpp>

#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

// -------------------------------------------------------------------
// Initialization & Lifecycle
// -------------------------------------------------------------------

// Default constructor: initializes the socket to an invalid, closed state.
Socket::Socket()
{
    m_Handle = INVALID_SOCKET;
    m_State = SocketState::Closed;
    m_LastReceiveResult = ReceiveResult::Success;
    m_LastReceiveClosed = false;
    m_LastReceiveTimeout = false;
    m_ReceiveTimeout = 5000;
    m_LastError = 0;
}

// Move constructor: transfers ownership of the native handle and state.
Socket::Socket(Socket&& other) noexcept
{
    std::lock_guard<std::mutex> lock(other.m_StateMutex);

    m_Handle = other.m_Handle;
    m_State = other.m_State;
    m_LastReceiveResult = other.m_LastReceiveResult;
    m_LastReceiveClosed = other.m_LastReceiveClosed;
    m_LastReceiveTimeout = other.m_LastReceiveTimeout;
    m_ReceiveTimeout = other.m_ReceiveTimeout;
    m_LastError = other.m_LastError;

    other.m_Handle = INVALID_SOCKET;
    other.m_State = SocketState::Closed;
}

// Destructor: ensures the underlying socket is closed.
Socket::~Socket()
{
    Close();
}

// Creates the native socket; returns true if already valid.
bool Socket::Create(int af, int type, int protocol)
{
    std::lock_guard<std::mutex> lock(m_StateMutex);

    if (m_Handle != INVALID_SOCKET)
    {
        return true;
    }

    m_Handle = ::socket(af, type, protocol);

    if (m_Handle == INVALID_SOCKET)
    {
        int errorCode = WSAGetLastError();
        m_LastError = errorCode;
        std::cout << "[NET_SOCK_ERR] Socket creation failed. Error Code: " << errorCode
                  << " | " << NetworkError::GetErrorMessage(errorCode) << std::endl;
        return false;
    }

    m_State = SocketState::Created;
    return true;
}

// Closes the native socket and resets the state to Closed.
void Socket::Close()
{
    std::lock_guard<std::mutex> lock(m_StateMutex);

    if (m_Handle == INVALID_SOCKET)
    {
        m_State = SocketState::Closed;
        return;
    }

    // Gracefully shut down both directions before releasing the handle.
    shutdown(m_Handle, SD_BOTH);
    closesocket(m_Handle);

    m_Handle = INVALID_SOCKET;
    m_State = SocketState::Closed;
}

// Returns true when the socket holds a valid native handle.
bool Socket::isValid() const
{
    std::lock_guard<std::mutex> lock(m_StateMutex);
    return m_Handle != INVALID_SOCKET;
}

// -------------------------------------------------------------------
// Client Connection & I/O Operations
// -------------------------------------------------------------------

// Connects the socket to the given remote address.
bool Socket::Connect(const Address& address)
{
    SOCKET handle = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        if (m_Handle == INVALID_SOCKET)
        {
            std::cout << "[NET_SOCK_ERR] Invalid socket descriptor for connect operation.\n";
            return false;
        }
        handle = m_Handle;
    }

    int result = connect(handle, address.GetSockAddr(), address.GetLength());

    if (result == SOCKET_ERROR)
    {
        int errorCode = WSAGetLastError();
        {
            std::lock_guard<std::mutex> lock(m_StateMutex);
            m_LastError = errorCode;
        }
        std::cout << "[NET_SOCK_ERR] Connection failed. Error Code: " << errorCode
                  << " | " << NetworkError::GetErrorMessage(errorCode) << std::endl;
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        m_State = SocketState::Connected;
    }

    return true;
}

// Sends the full buffer, looping until all bytes are written or an error occurs.
int Socket::Send(const void* buffer, int length)
{
    SOCKET handle = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        if (m_Handle == INVALID_SOCKET)
        {
            std::cout << "[NET_SOCK_ERR] Invalid socket handle for send operation.\n";
            return SOCKET_ERROR;
        }
        if (m_State != SocketState::Connected)
        {
            std::cout << "[NET_SOCK_ERR] Cannot Send: Socket is not Connected" << std::endl;
            return SOCKET_ERROR;
        }
        handle = m_Handle;
    }

    std::cout << "[NET_SOCK_DBG] Send called. Buffer length: " << length << std::endl;

    const char* data = static_cast<const char*>(buffer);
    int totalSent = 0;

    while (totalSent < length)
    {
        int sent = send(handle, data + totalSent, length - totalSent, 0);

        if (sent == SOCKET_ERROR)
        {
            int errorCode = WSAGetLastError();
            {
                std::lock_guard<std::mutex> lock(m_StateMutex);
                m_LastError = errorCode;
            }
            std::cout << "[NET_SOCK_ERR] Raw send failed. Error Code: " << errorCode
                      << " | " << NetworkError::GetErrorMessage(errorCode) << std::endl;
            return SOCKET_ERROR;
        }

        if (sent == 0)
        {
            break;
        }

        totalSent += sent;
    }

    return totalSent;
}

// Receives up to 'length' bytes from a connected socket.
int Socket::Receive(void* buffer, int length)
{
    SOCKET handle = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        if (m_State != SocketState::Connected || m_Handle == INVALID_SOCKET)
        {
            std::cout << "[NET_SOCK_ERR] Cannot receive data. Socket is not connected.\n";
            return SOCKET_ERROR;
        }
        handle = m_Handle;
    }

    return recv(handle, static_cast<char*>(buffer), length, 0);
}

// -------------------------------------------------------------------
// Server Binding & Listening
// -------------------------------------------------------------------

// Binds the socket to the given local address.
bool Socket::Bind(const Address& address)
{
    SOCKET handle = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        if (m_Handle == INVALID_SOCKET)
        {
            std::cout << "[NET_SOCK_ERR] Invalid socket handle for bind operation!\n";
            return false;
        }
        handle = m_Handle;
    }

    int result = bind(handle, address.GetSockAddr(), address.GetLength());

    if (result == SOCKET_ERROR)
    {
        int errorCode = WSAGetLastError();
        {
            std::lock_guard<std::mutex> lock(m_StateMutex);
            m_LastError = errorCode;
        }
        std::cout << "[NET_SOCK_ERR] Bind failed. Error Code: " << errorCode
                  << " | " << NetworkError::GetErrorMessage(errorCode) << std::endl;
        return false;
    }

    return true;
}

// Puts the socket into listening mode with the given backlog.
bool Socket::Listen(int backlog)
{
    SOCKET handle = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        if (m_Handle == INVALID_SOCKET)
        {
            std::cout << "[NET_SOCK_ERR] Invalid socket handle for listen operation!\n";
            return false;
        }
        handle = m_Handle;
    }

    int result = listen(handle, backlog);

    if (result == SOCKET_ERROR)
    {
        int errorCode = WSAGetLastError();
        {
            std::lock_guard<std::mutex> lock(m_StateMutex);
            m_LastError = errorCode;
        }
        std::cout << "[NET_SOCK_ERR] Listen failed. Error Code: " << errorCode
                  << " | " << NetworkError::GetErrorMessage(errorCode) << std::endl;
        return false;
    }

    return true;
}

// Accepts an incoming client connection and returns it as a new Socket.
Socket Socket::Accept()
{
    Socket client;

    SOCKET handle = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        if (m_Handle == INVALID_SOCKET)
        {
            std::cout << "[NET_SOCK_ERR] Invalid socket handle for accept operation!\n";
            return client;
        }
        handle = m_Handle;
    }

    SOCKET clientHandle = accept(handle, nullptr, nullptr);

    if (clientHandle == INVALID_SOCKET)
    {
        int errorCode = WSAGetLastError();
        {
            std::lock_guard<std::mutex> lock(m_StateMutex);
            m_LastError = errorCode;
        }
        std::cout << "[NET_SOCK_ERR] Accept failed. Error Code: " << errorCode
                  << " | " << NetworkError::GetErrorMessage(errorCode) << std::endl;
        return client;
    }

    {
        std::lock_guard<std::mutex> lock(client.m_StateMutex);
        client.m_Handle = clientHandle;
        client.m_State = SocketState::Connected;
    }

    return client;
}

// -------------------------------------------------------------------
// Stream Buffer Processing & Packet Serialization
// -------------------------------------------------------------------

// Reads exactly 'size' bytes into the buffer, handling partial reads.
bool Socket::ReceiveExact(void* data, int size)
{
    SOCKET handle = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        if (m_Handle == INVALID_SOCKET)
        {
            std::cout << "[NET_SOCK_ERR] Invalid socket handle for ReceiveExact operation!\n";
            return false;
        }
        handle = m_Handle;
        m_LastReceiveClosed = false;
        m_LastReceiveTimeout = false;
    }

    char* buffer = static_cast<char*>(data);
    int totalReceived = 0;

    while (totalReceived < size)
    {
        std::cout << "[NET_SOCK_DBG] Waiting to receive: " << size - totalReceived << " bytes" << std::endl;
        int received = recv(handle, buffer + totalReceived, size - totalReceived, 0);

        if (received == SOCKET_ERROR)
        {
            int errorCode = WSAGetLastError();
            {
                std::lock_guard<std::mutex> lock(m_StateMutex);
                m_LastError = errorCode;
                m_LastReceiveClosed = false;
                if (errorCode == WSAETIMEDOUT)
                {
                    m_LastReceiveTimeout = true;
                    std::cout << "[NET_SOCK_WARN] Receive timeout." << std::endl;
                }
                else
                {
                    std::cout << "[NET_SOCK_ERR] Receive Failed. Error Code: " << errorCode
                              << " | " << NetworkError::GetErrorMessage(errorCode) << std::endl;
                }
            }
            return false;
        }

        if (received == 0)
        {
            {
                std::lock_guard<std::mutex> lock(m_StateMutex);
                m_LastReceiveClosed = true;
            }
            std::cout << "[NET_SOCK_INFO] Connection closed gracefully by peer. " << std::endl;
            return false;
        }

        totalReceived += received;
        std::cout << "[NET_SOCK_DBG] Received chunk: " << received << " | Total: " << totalReceived << std::endl;
    }

    return true;
}

// Serializes and sends a protocol header followed by the packet payload.
bool Socket::SendPacket(const Packet& packet)
{
    ProtocolHeader header;
    header.Type = packet.GetType();
    header.PayloadSize = static_cast<std::uint32_t>(packet.Size());

    if (!ProtocolValidator::Validate(header))
    {
        std::cout << "[NET_SOCK_ERR] Invalid Packet Header validation." << std::endl;
        return false;
    }

    std::vector<std::uint8_t> headerData = ProtocolSerializer::Serialize(header);

    int headerSent = Send(headerData.data(), static_cast<int>(headerData.size()));

    if (headerSent != static_cast<int>(headerData.size()))
    {
        int errorCode = WSAGetLastError();
        std::cout << "[NET_SOCK_ERR] Failed Sending Protocol Header." << std::endl;
        std::cout << "[NET_SOCK_DBG] Expected: " << headerData.size() << " bytes" << std::endl;
        std::cout << "[NET_SOCK_DBG] Sent: " << headerSent << " bytes" << std::endl;
        std::cout << "[NET_SOCK_DBG] WSA Error Code: " << errorCode
                  << " | " << NetworkError::GetErrorMessage(errorCode) << std::endl;
        return false;
    }

    if (packet.Size() > 0)
    {
        int dataSent = Send(packet.Data(), static_cast<int>(packet.Size()));
        if (dataSent != static_cast<int>(packet.Size()))
        {
            std::cout << "[NET_SOCK_ERR] Failed Sending Packet Data Payload." << std::endl;
            return false;
        }
    }

    return true;
}

// Receives a header and payload, validating the header before accepting data.
bool Socket::ReceivePacket(Packet& packet)
{
    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        m_LastReceiveTimeout = false;
        m_LastReceiveResult = ReceiveResult::Success;
        m_LastReceiveClosed = false;
    }

    std::vector<std::uint8_t> headerData(12);

    std::cout << "[NET_SOCK_INFO] Waiting Protocol Header..." << std::endl;

    if (!ReceiveExact(headerData.data(), static_cast<int>(headerData.size())))
    {
        std::lock_guard<std::mutex> lock(m_StateMutex);

        if (m_LastReceiveClosed)
        {
            m_LastReceiveResult = ReceiveResult::ConnectionClosed;
        }
        else if (m_LastReceiveTimeout)
        {
            m_LastReceiveResult = ReceiveResult::Timeout;
        }
        else
        {
            m_LastReceiveResult = ReceiveResult::ReceiveFailed;
        }
        return false;
    }

    ProtocolHeader header;

    if (!ProtocolSerializer::Deserialize(headerData, header))
    {
        std::cout << "[NET_SOCK_ERR] Failed to Deserialize Protocol Header." << std::endl;
        std::lock_guard<std::mutex> lock(m_StateMutex);
        m_LastReceiveResult = ReceiveResult::InvalidHeader;
        return false;
    }

    if (header.Magic != 0x4153434B)
    {
        std::cout << "[NET_PROTO_ERR] Invalid Protocol magic." << std::endl;
        std::lock_guard<std::mutex> lock(m_StateMutex);
        m_LastReceiveResult = ReceiveResult::InvalidHeader;
        return false;
    }

    if (!IsValidPacketType(header.Type))
    {
        std::cout << "[NET_PROTO_ERR] Unsupported packet type." << std::endl;
        std::lock_guard<std::mutex> lock(m_StateMutex);
        m_LastReceiveResult = ReceiveResult::InvalidHeader;
        return false;
    }

    if (header.Version != 1)
    {
        std::cout << "[NET_PROTO_ERR] Unsupported Protocol Version: " << header.Version << std::endl;
        std::lock_guard<std::mutex> lock(m_StateMutex);
        m_LastReceiveResult = ReceiveResult::InvalidHeader;
        return false;
    }

    if (header.PayloadSize > MAX_PAYLOAD_SIZE)
    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        m_LastReceiveResult = ReceiveResult::PayloadTooLarge;
        std::cout << "[NET_SOCK_ERR] Payload Size Exceeds Maximum Allowed Size. Received: "
                  << header.PayloadSize << " / Maximum: " << MAX_PAYLOAD_SIZE << std::endl;
        return false;
    }

    if (!ProtocolValidator::Validate(header))
    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        m_LastReceiveResult = ReceiveResult::InvalidHeader;
        std::cout << "[NET_SOCK_ERR] Invalid Protocol Header Validation Failed." << std::endl;
        return false;
    }

    std::cout << "[NET_SOCK_INFO] Packet Type : " << static_cast<int>(header.Type)
              << " (" << packet.PacketTypeToStirng(static_cast<PacketType>(header.Type)) << ")" << std::endl;
    std::cout << "[NET_SOCK_INFO] Payload Size: " << header.PayloadSize << " bytes" << std::endl;

    std::vector<std::uint8_t> buffer(header.PayloadSize);

    if (header.PayloadSize > 0)
    {
        if (!ReceiveExact(buffer.data(), static_cast<int>(buffer.size())))
        {
            std::lock_guard<std::mutex> lock(m_StateMutex);
            m_LastReceiveResult = m_LastReceiveTimeout
                                    ? ReceiveResult::Timeout
                                    : (m_LastReceiveClosed ? ReceiveResult::ConnectionClosed
                                                           : ReceiveResult::ReceiveFailed);
            return false;
        }
    }

    packet.clear();
    packet.SetType(header.Type);

    if (header.PayloadSize > 0)
    {
        packet.WriteBytes(buffer.data(), header.PayloadSize);
    }

    {
        std::lock_guard<std::mutex> lock(m_StateMutex);
        m_LastReceiveResult = ReceiveResult::Success;
    }

    return true;
}

// -------------------------------------------------------------------
// Error Handling & State Querying
// -------------------------------------------------------------------

// Returns the last recorded socket error code.
int Socket::GetLastError() const
{
    std::lock_guard<std::mutex> lock(m_StateMutex);
    return m_LastError;
}

// Resets the stored socket error code to zero.
void Socket::ClearLastError()
{
    std::lock_guard<std::mutex> lock(m_StateMutex);
    m_LastError = 0;
}

// Stores the given socket error code.
void Socket::SetLastError(int error)
{
    std::lock_guard<std::mutex> lock(m_StateMutex);
    m_LastError = error;
}

// Move assignment: closes the current socket then steals the other's handle.
Socket& Socket::operator=(Socket&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    Close();

    std::lock_guard<std::mutex> lock(other.m_StateMutex);

    m_Handle = other.m_Handle;
    m_State = other.m_State;
    m_LastReceiveResult = other.m_LastReceiveResult;
    m_LastReceiveClosed = other.m_LastReceiveClosed;
    m_LastReceiveTimeout = other.m_LastReceiveTimeout;
    m_ReceiveTimeout = other.m_ReceiveTimeout;
    m_LastError = other.m_LastError;

    other.m_Handle = INVALID_SOCKET;
    other.m_State = SocketState::Closed;

    return *this;
}

// Returns the result of the most recent receive operation.
Socket::ReceiveResult Socket::GetLastReceiveResult() const
{
    std::lock_guard<std::mutex> lock(m_StateMutex);
    return m_LastReceiveResult;
}

// Returns true when the socket is in the Connected state.
bool Socket::IsConnected() const
{
    std::lock_guard<std::mutex> lock(m_StateMutex);
    return m_State == SocketState::Connected;
}

// Applies the given receive timeout to the underlying socket.
void Socket::SetReceiveTimeout(int milliseconds)
{
    std::lock_guard<std::mutex> lock(m_StateMutex);

    if (m_Handle == INVALID_SOCKET)
    {
        return;
    }

    if (milliseconds < 0)
    {
        milliseconds = 0;
    }

    m_ReceiveTimeout = milliseconds;

    int result = setsockopt(m_Handle, SOL_SOCKET, SO_RCVTIMEO,
                            reinterpret_cast<const char*>(&milliseconds),
                            sizeof(milliseconds));

    if (result == SOCKET_ERROR)
    {
        int errorCode = WSAGetLastError();
        m_LastError = errorCode;
        std::cout << "[NET_SOCK_ERR] setsockopt(SO_RCVTIMEO) failed. Error Code: "
                  << errorCode << " | " << NetworkError::GetErrorMessage(errorCode) << std::endl;
    }
}

// Returns the configured receive timeout in milliseconds.
int Socket::GetReceiveTimeout() const
{
    std::lock_guard<std::mutex> lock(m_StateMutex);
    return m_ReceiveTimeout;
}

// Returns true if the given packet type is supported by this engine.
bool Socket::IsValidPacketType(PacketType type) const
{
    switch (type)
    {
    case PacketType::Handshake:
    case PacketType::Ping:
    case PacketType::Pong:
    case PacketType::Command:
    case PacketType::Response:
    case PacketType::Beacon:
        return true;

    case PacketType::Unknown:
    default:
        return false;
    }
}

// Automatically replies with a Pong when a Ping packet is received.
bool Socket::ProcessPingPong(const Packet& receivedPacket)
{
    if (receivedPacket.GetType() == PacketType::Ping)
    {
        std::cout << "[NET_SOCK_INFO] Ping received. Sending automatic Pong Response" << std::endl;
        Packet pongPacket;
        pongPacket.SetType(PacketType::Pong);
        return SendPacket(pongPacket);
    }
    return false;
}

// Returns the current system time in seconds since epoch.
static uint64_t GetCurrentTimestamp()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
}

// Background loop that periodically checks for timed-out client sessions.
void Socket::TimeoutMonitorLoop(uint64_t intervalSeconds)
{
    while (m_isRunning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));

        if (!m_isRunning)
        {
            break;
        }

        CheckSessionTimeouts(15);
    }
}

// Creates, binds, and listens; starts the timeout monitor thread.
bool Socket::StartServer(const std::string& ip, uint16_t port)
{
    if (!Create()) return false;

    Address serverAddress(ip, port);
    if (!Bind(serverAddress)) return false;
    if (!Listen()) return false;

    m_isRunning = true;

    if (!m_timeoutThread.joinable())
    {
        m_timeoutThread = std::thread(&Socket::TimeoutMonitorLoop, this, 5);
    }

    std::cout << "[NET_ENGINE] Multi-Client C2 Engine Started on : " << ip << ":" << port << std::endl;
    return true;
}

// Stops the server loop, closes the listening socket, and joins client threads.
void Socket::StopServer()
{
    m_isRunning = false;

    Close();

    std::vector<std::thread> pendingThreads;

    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        pendingThreads.swap(m_clientThreads);
    }

    for (std::thread& th : pendingThreads)
    {
        if (th.joinable())
        {
            th.join();
        }
    }

    if (m_timeoutThread.joinable())
    {
        m_timeoutThread.join();
    }

    // Close any remaining client session sockets.
    {
        std::lock_guard<std::mutex> sessionsLock(m_sessionsMutex);
        for (auto& session : m_activeSessions)
        {
            if (session.SocketHandle != INVALID_SOCKET)
            {
                shutdown(session.SocketHandle, SD_BOTH);
            }
        }
        m_activeSessions.clear();
    }
}

// Accepts incoming clients in a loop and dispatches each to a handler thread.
void Socket::ListenForClients(PacketCallback onPacketReceived)
{
    while (m_isRunning)
    {
        Socket clientSocket = Accept();

        if (clientSocket.isValid())
        {
            SOCKET rawClientSocket = INVALID_SOCKET;

            {
                std::lock_guard<std::mutex> lock(clientSocket.m_StateMutex);
                rawClientSocket = clientSocket.m_Handle;
                clientSocket.m_Handle = INVALID_SOCKET;
            }

            std::cout << "[NET_ENGINE] New Agent Connected! Spawning Thread..." << std::endl;

            std::lock_guard<std::mutex> lock(m_clientMutex);
            m_clientThreads.emplace_back(&Socket::HandleClientThread, this, rawClientSocket, onPacketReceived);
        }
        else
        {
            if (!m_isRunning)
            {
                break;
            }
        }
    }
}

// Handles a single client connection until disconnect or shutdown.
void Socket::HandleClientThread(SOCKET clientSocketHandle, PacketCallback onPacketReceived)
{
    Socket client;

    {
        std::lock_guard<std::mutex> lock(client.m_StateMutex);
        client.m_Handle = clientSocketHandle;
        client.m_State = SocketState::Connected;
    }

    uint32_t currentSessionID = 0;

    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);
        currentSessionID = m_nextSessionId++;

        ClientSession session;
        session.SessionID = currentSessionID;
        session.SocketHandle = clientSocketHandle;
        session.LastBeaconTime = GetCurrentTimestamp();
        session.IsActive = true;
        m_activeSessions.push_back(session);
    }

    std::cout << "[NET_ENGINE] Registered Active Session ID: " << currentSessionID << std::endl;

    while (m_isRunning)
    {
        Packet packet;

        if (!client.ReceivePacket(packet))
        {
            std::cout << "[NET_ENGINE] Session: " << currentSessionID << " Disconnected." << std::endl;
            break;
        }

        if (packet.GetType() == PacketType::Ping)
        {
            client.ProcessPingPong(packet);
        }
        else if (packet.GetType() == PacketType::Beacon)
        {
            std::lock_guard<std::mutex> lock(m_sessionsMutex);

            for (auto& s : m_activeSessions)
            {
                if (s.SessionID == currentSessionID)
                {
                    s.LastBeaconTime = GetCurrentTimestamp();
                    std::cout << "[NET_ENGINE] Beacon/Hearbeat Updated for Session: "
                              << currentSessionID << std::endl;
                    break;
                }
            }
        }

        if (onPacketReceived)
        {
            onPacketReceived(client, packet);
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);

        for (auto it = m_activeSessions.begin(); it != m_activeSessions.end(); ++it)
        {
            if (it->SessionID == currentSessionID)
            {
                m_activeSessions.erase(it);
                break;
            }
        }
    }

    client.Close();
}

// Returns a copy of all currently active client sessions.
std::vector<Socket::ClientSession> Socket::GetActiveSessions()
{
    std::lock_guard<std::mutex> lock(m_sessionsMutex);
    return m_activeSessions;
}

// Sends a command packet to a specific active session by ID.
bool Socket::SendCommandToSession(uint32_t sessionID, const Packet& commandPacket)
{
    SOCKET targetHandle = INVALID_SOCKET;

    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);

        for (auto& session : m_activeSessions)
        {
            if (session.SessionID == sessionID && session.IsActive)
            {
                targetHandle = session.SocketHandle;
                break;
            }
        }
    }

    if (targetHandle == INVALID_SOCKET)
    {
        return false;
    }

    // Send outside the session lock so a slow peer cannot stall other sessions.
    Socket target;
    {
        std::lock_guard<std::mutex> lock(target.m_StateMutex);
        target.m_Handle = targetHandle;
        target.m_State = SocketState::Connected;
    }

    bool result = target.SendPacket(commandPacket);

    {
        std::lock_guard<std::mutex> lock(target.m_StateMutex);
        target.m_Handle = INVALID_SOCKET;
        target.m_State = SocketState::Closed;
    }

    return result;
}

// Removes sessions whose last beacon exceeds the given timeout.
void Socket::CheckSessionTimeouts(uint64_t timeoutSeconds)
{
    std::vector<SOCKET> expiredHandles;

    {
        std::lock_guard<std::mutex> lock(m_sessionsMutex);

        uint64_t currentTime = GetCurrentTimestamp();

        for (auto it = m_activeSessions.begin(); it != m_activeSessions.end();)
        {
            if (currentTime - it->LastBeaconTime > timeoutSeconds)
            {
                std::cout << "[NET_ENGINE] Session ID : " << it->SessionID
                          << " Timed out (No Beacon). Removing..." << std::endl;
                expiredHandles.push_back(it->SocketHandle);
                it = m_activeSessions.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Gracefully shut down outside the session lock to avoid blocking other threads.
    for (SOCKET handle : expiredHandles)
    {
        if (handle != INVALID_SOCKET)
        {
            shutdown(handle, SD_BOTH);
        }
    }
}