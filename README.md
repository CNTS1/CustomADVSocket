# CustomADVSocket
A lightweight C++17 TCP networking library for Windows built on Winsock2. Provides a thread-safe socket wrapper, length-prefixed packet framing, protocol header validation, and a small multi-client session manager suitable for command/control or agent-style servers.



# ⚡ NetEngine
### A Custom C++ TCP Networking Engine for Windows

<p align="center">
  <strong>A structured networking foundation built around Winsock2, custom packet framing, protocol validation, and multi-client session management.</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-blue?style=for-the-badge&logo=c%2B%2B" alt="C++17">
  <img src="https://img.shields.io/badge/Platform-Windows-0078D6?style=for-the-badge&logo=windows" alt="Windows">
  <img src="https://img.shields.io/badge/Transport-TCP-2E8B57?style=for-the-badge" alt="TCP">
  <img src="https://img.shields.io/badge/Network_API-Winsock2-555555?style=for-the-badge" alt="Winsock2">
  <img src="https://img.shields.io/badge/Status-Under_Development-orange?style=for-the-badge" alt="Under Development">
</p>

---

## 📌 Overview

**NetEngine** is a custom C++ networking library developed on top of the Windows Winsock2 API.

It was originally created as a personal engineering project, with a focus on understanding and building the networking infrastructure rather than relying entirely on high-level networking abstractions.

As development progressed, the project evolved into a more structured networking engine, introducing a custom packet format, protocol handling, socket lifecycle management, and multi-client session infrastructure.

Although it was initially intended for personal use, I decided to publish it so that other developers can explore its architecture, inspect its implementation, and follow its development.

NetEngine is currently a work in progress. Its architecture and implementation continue to evolve, and some protocol components have not yet reached their intended level of completeness.

> **The goal is straightforward:** build a networking foundation with explicit control over sockets, packet structure, protocol handling, and connection management.

## ✨ Core Features

### 🌐 Socket Infrastructure
- Custom TCP socket wrapper built on Windows Winsock2.
- Socket creation, binding, listening, accepting, connecting, and closing.
- Explicit socket lifecycle and connection state management.
- Move semantics for socket ownership transfer.
- Low-level send and receive operations.
- Configurable receive timeout interface.
- Automatic resource cleanup through the socket destructor.

### 📦 Packet Management
- Custom packet abstraction backed by a byte buffer.
- Raw byte writing and reading.
- 32-bit signed and unsigned integer serialization.
- Length-prefixed string serialization.
- Packet type identification.
- Read-position tracking.
- Bounds checking before reading packet data.
- Payload size inspection and buffer management.

### 🔄 Custom Protocol Layer

NetEngine implements its own protocol header and serialization layer.

The current protocol header uses a fixed 12-byte structure containing:

| Field | Size | Description |
|---|---:|---|
| Magic | 4 bytes | Protocol signature |
| Version | 2 bytes | Protocol version |
| Packet Type | 2 bytes | Packet identifier |
| Payload Size | 4 bytes | Payload length |

The header is serialized into network byte order before transmission.

Additional protocol capabilities include:

- Header serialization and deserialization.
- Magic signature verification.
- Protocol version validation.
- Packet type validation.
- Payload size validation.
- Separation between protocol serialization and semantic validation.

### 🛡️ Network Data Handling
- Exact-size receive handling through `ReceiveExact()`.
- Handling of partial TCP reads.
- Explicit packet framing over TCP streams.
- Header validation before accepting packet payloads.
- Configurable maximum packet payload size in the socket receive path.
- Distinct receive results for connection closure, invalid headers, oversized payloads, timeouts, and receive failures.

### 🔀 Multi-Client Management
- Built-in TCP server startup and shutdown operations.
- Accept loop for incoming client connections.
- Dedicated client handler threads.
- Client session registration and tracking.
- Session identifiers.
- Active session enumeration.
- Packet callbacks for application-level processing.
- Sending packets to a selected session.
- Beacon-based heartbeat tracking.
- Background session timeout monitoring.

### 🔐 Synchronization & Resource Management
- Mutex-based protection of socket state.
- Separate synchronization for session management.
- Atomic server-running state.
- Controlled transfer of socket ownership.
- Explicit shutdown and thread joining during server shutdown.

### ⚙️ Error Handling
- Winsock error-code retrieval.
- Human-readable network error messages.
- Last socket error tracking.
- Dedicated packet receive result states.
- Error handling across major socket operations.

---

## 🧩 Packet Types

The protocol currently declares the following packet identifiers:

| Packet Type | Identifier |
|---|---:|
| Unknown | 0 |
| Handshake | 1 |
| Ping | 2 |
| Pong | 3 |
| Command | 4 |
| Response | 5 |
| Beacon | 6 |
| CommandReq | 7 |
| CommandResp | 8 |
| Disconnect | 9 |

**Implementation note:** Declaring a packet type does not necessarily mean its complete handling is implemented. The current socket receive validation accepts Handshake, Ping, Pong, Command, Response, and Beacon. The remaining declared types are not yet fully integrated into that receive path.

---

## 🏗️ Architecture

NetEngine is organized into separate components to keep networking, protocol processing, and utility functionality distinct.

```text
NetEngine-Beta/
│
├── include/
│   └── NetEngine/
│       ├── Core/
│       │   ├── Error/
│       │   ├── Protocol/
│       │   └── Utils/
│       │
│       └── Network/
│           ├── Address.hpp
│           ├── Packet.hpp
│           ├── Socket.hpp
│           └── Winsock.hpp
│
├── src/
│   ├── Network/
│   ├── Protocol/
│   └── Utils/
│
└── lib/
    └── libNetEngine.a
```

### Component Responsibilities

| Component | Responsibility |
|---|---|
| Network | Socket operations, addressing, and packet transport |
| Packet | Packet buffer management and data serialization |
| Protocol | Header serialization and validation |
| Utils | Byte-order conversion and network utilities |
| Core/Error | Network error message translation |

---

## 🚀 Quick Start

### Requirements

- Windows operating system.
- C++17-compatible compiler.
- Windows Winsock2 headers and libraries.
- Winsock2 library (`Ws2_32.lib`).

### Linking

For MinGW:

```bash
-lws2_32
```

For MSVC:

```text
Ws2_32.lib
```

The repository currently includes a static library artifact under `lib/`. Its compatibility should be checked against your compiler and build configuration.

There is currently no documented automated CMake installation workflow in this project directory. Integrate the source files or compatible library artifact into your own build configuration.

### Basic Client Example

```cpp
#include <iostream>

#include <NetEngine/Network/Winsock.hpp>
#include <NetEngine/Network/Socket.hpp>
#include <NetEngine/Network/Address.hpp>
#include <NetEngine/Network/Packet.hpp>

int main()
{
    if (!Winsock::Initialize())
        return -1;

    Socket client;

    if (!client.Create())
    {
        Winsock::Cleanup();
        return -1;
    }

    Address serverAddress("127.0.0.1", 8080);

    if (!client.Connect(serverAddress))
    {
        client.Close();
        Winsock::Cleanup();
        return -1;
    }

    Packet packet;
    packet.SetType(PacketType::Command);
    packet.WriteString("Hello from NetEngine");

    if (!client.SendPacket(packet))
    {
        std::cerr << "Packet transmission failed.\n";
    }

    client.Close();
    Winsock::Cleanup();

    return 0;
}
```

This example demonstrates the existing client-side API. A compatible server must be listening at the specified address and understand the same protocol format.

---

## 📚 Documentation

The complete API reference, including function descriptions, parameters, return values, and implementation details, is available through the project's documentation website.

<p align="center">
  <a href="[YOUR_DOCUMENTATION_URL](https://kalikali-lgtml.github.io/NetEngineLib/)">
    <strong>📖 Explore the Full NetEngine API Documentation</strong>
  </a>
</p>

**[Documentation website:](https://kalikali-lgtml.github.io/NetEngineLib/)** 

---

## 🚧 Development Status

<kbd>Under Active Development</kbd>

NetEngine is not a finalized networking framework.

The project is still evolving, and some parts of the implementation require additional development, testing, and refinement.

Current development considerations include:

- Some declared packet types are not fully implemented in the receive pipeline.
- Protocol functionality is still being expanded.
- Additional testing and validation are required.
- API design and internal architecture may change.
- Additional documentation and usage examples are being prepared.

The presence of a declared interface or enum value should not be interpreted as proof of a complete implementation.

---

## ⚠️ Security Considerations & Limitations

Security and correctness have been important considerations throughout the development process.

Several issues identified in earlier implementations were corrected, with particular attention to socket state handling, resource lifetime, synchronization, and packet processing.

However, NetEngine has **not undergone an independent security audit or formal verification**.

Please consider the following limitations:

- NetEngine does not currently provide built-in TLS or end-to-end encryption.
- Packet framing and protocol validation do not provide cryptographic authentication.
- Network payloads should not be considered confidential or authenticated by default.
- The packet serialization helpers for 32-bit values currently use native host byte order; applications requiring cross-platform interoperability should define and enforce their own payload encoding.
- The generic protocol validator and socket receive path currently apply different payload-size limits. The socket receive path enforces a 64 KiB limit.
- Thread synchronization does not automatically make every possible concurrent socket operation safe.

**Important:** Do not expose this implementation to untrusted networks or use it to protect sensitive communications without adding an appropriate authenticated security layer and conducting the necessary testing.

For security-sensitive applications, use established cryptographic protocols and independently reviewed implementations.

---

## 🎯 Project Philosophy

NetEngine was built around a simple engineering principle:

**Understand the underlying mechanisms. Control the implementation. Improve it through testing and iteration.**

Rather than treating networking as a black box, this project explores the components behind a practical TCP communication layer:

- Socket lifecycle management.
- Stream-oriented transport.
- Packet framing.
- Binary serialization.
- Protocol structure.
- Connection state.
- Concurrent client handling.
- Error reporting and resource ownership.

The project is primarily a personal engineering effort shared publicly for transparency, learning, and continued development.

---

## 🗺️ Roadmap

The project is continuously evolving. Future work may include:

- Completing the remaining packet type implementations.
- Expanding protocol validation.
- Improving packet serialization consistency.
- Additional reliability and stress testing.
- More comprehensive API documentation.
- Further synchronization and lifecycle improvements.
- Expanded examples and integration guidance.

This roadmap represents development directions, not completed features.

---

## 🤝 Contributions & Feedback

The project is publicly available for developers interested in reviewing its implementation and following its progress.

Suggestions, technical feedback, and bug reports are welcome.

If you identify a problem or have an improvement in mind, feel free to open an issue.

---

## ⚖️ License

Please refer to the repository's license information before redistributing or incorporating NetEngine into other projects.

---

<p align="center">
  <strong>NetEngine</strong>
  <br>
  Built with C++ and Winsock2.
  <br>
  <sub>A personal engineering project, shared publicly and continuously evolving.</sub>
</p>
