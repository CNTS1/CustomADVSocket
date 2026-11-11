#include "NetEngine/Core/Error/NetworkError.hpp"
#include <winsock2.h>

std::string NetworkError::GetErrorMessage(int errorCode)
{
    switch (errorCode)
    {
        // ===================================================================
        // 1. SYSTEM & INITIALIZATION ERRORS
        // ===================================================================
        case 0:
            return "[NET_OK] Operation completed successfully.";
        case WSANOTINITIALISED:
            return "[NET_SYS_NOT_INIT] WSAStartup was not performed or failed.";
        case WSASYSNOTREADY:
            return "[NET_SYS_NOT_READY] Underlying network subsystem is unavailable.";
        case WSAVERNOTSUPPORTED:
            return "[NET_SYS_VER_UNSUPP] Requested Winsock DLL version is not supported.";
        case WSAEINVALIDPROCTABLE:
            return "[NET_SYS_BAD_PROCTABLE] Invalid procedure table from service provider.";
        case WSAEINVALIDPROVIDER:
            return "[NET_SYS_BAD_PROVIDER] Service provider returned an invalid version.";
        case WSAEPROVIDERFAILEDINIT:
            return "[NET_SYS_PROVIDER_FAIL] Service provider failed to initialize.";

        // ===================================================================
        // 2. CONNECTION & DISCONNECTION STATES
        // ===================================================================
        case WSAECONNREFUSED:
            return "[NET_CONN_REFUSED] Connection actively refused by target server.";
        case WSAETIMEDOUT:
            return "[NET_CONN_TIMEOUT] Connection attempt timed out.";
        case WSAECONNRESET:
            return "[NET_CONN_RESET] Connection forcibly closed by remote peer.";
        case WSAECONNABORTED:
            return "[NET_CONN_ABORTED] Software aborted connection due to timeout/error.";
        case WSAENETRESET:
            return "[NET_CONN_NETRESET] Connection dropped because keep-alive detected failure.";
        case WSAESHUTDOWN:
            return "[NET_CONN_SHUTDOWN] Cannot send/receive data after socket shutdown.";
        case WSAEDISCON:
            return "[NET_CONN_DISCONNECT] Graceful disconnect initiated by remote party.";

        // ===================================================================
        // 3. SOCKET STATE & ASYNCHRONOUS I/O
        // ===================================================================
        case WSAENOTSOCK:
            return "[NET_SOCK_INVALID] Provided handle is not a valid socket descriptor.";
        case WSAEISCONN:
            return "[NET_SOCK_ISCONN] Socket is already connected.";
        case WSAENOTCONN:
            return "[NET_SOCK_NOTCONN] Socket is not connected to any remote endpoint.";
        case WSAEWOULDBLOCK:
            return "[NET_SOCK_WOULDBLOCK] Non-blocking operation would block execution.";
        case WSAEINPROGRESS:
            return "[NET_SOCK_INPROGRESS] Blocking Winsock call is executing in background.";
        case WSAEALREADY:
            return "[NET_SOCK_ALREADY] Non-blocking connection attempt already in progress.";
        case WSAECANCELLED:
            return "[NET_SOCK_CANCELLED] Asynchronous lookup or operation was cancelled.";

        // ===================================================================
        // 4. ADDRESSING & PORT MANAGEMENT
        // ===================================================================
        case WSAEADDRINUSE:
            return "[NET_ADDR_INUSE] Address/Port is already bound by another socket.";
        case WSAEADDRNOTAVAIL:
            return "[NET_ADDR_UNAVAIL] Cannot assign requested address (Invalid IP/Interface).";
        case WSAEDESTADDRREQ:
            return "[NET_ADDR_REQ] Destination address is required for this operation.";
        case WSAEAFNOSUPPORT:
            return "[NET_ADDR_AF_UNSUPP] Address family is not supported by protocol.";

        // ===================================================================
        // 5. MEMORY, BUFFERS & ARGUMENTS
        // ===================================================================
        case WSAEINVAL:
            return "[NET_ARG_INVALID] Invalid argument supplied to API call.";
        case WSAEFAULT:
            return "[NET_MEM_FAULT] Bad pointer address or invalid memory access.";
        case WSAEMSGSIZE:
            return "[NET_BUF_OVERFLOW] Message size exceeds socket/buffer limit.";
        case WSAENOBUFS:
            return "[NET_MEM_NOBUFS] Insufficient buffer space available in system.";
        case WSAEMFILE:
            return "[NET_SYS_MFILE] Reached maximum allowed open sockets for process.";
        case WSAEACCES:
            return "[NET_SEC_DENIED] Access permission denied (Broadcast without flag?).";

        // ===================================================================
        // 6. ROUTING & NETWORK INFRASTRUCTURE
        // ===================================================================
        case WSAENETDOWN:
            return "[NET_ROUTE_NETDOWN] Network subsystem has completely failed.";
        case WSAENETUNREACH:
            return "[NET_ROUTE_NETUNREACH] Network cannot be reached from this host.";
        case WSAEHOSTUNREACH:
            return "[NET_ROUTE_HOSTUNREACH] Remote host is unreachable.";
        case WSAEHOSTDOWN:
            return "[NET_ROUTE_HOSTDOWN] Target host is down or unresponsive.";

        // ===================================================================
        // 7. PROTOCOL & OPTION ERRORS
        // ===================================================================
        case WSAEPROTONOSUPPORT:
            return "[NET_PROTO_UNSUPP] Requested protocol is not supported.";
        case WSAEPROTOTYPE:
            return "[NET_PROTO_WRONG_TYPE] Wrong protocol type for requested socket.";
        case WSAENOPROTOOPT:
            return "[NET_PROTO_BAD_OPT] Unknown, invalid, or unsupported socket option.";
        case WSAESOCKTNOSUPPORT:
            return "[NET_PROTO_SOCK_UNSUPP] Socket type is not supported in this domain.";
        case WSAEPFNOSUPPORT:
            return "[NET_PROTO_PF_UNSUPP] Protocol family is not supported.";

        // ===================================================================
        // 8. DNS & NAME RESOLUTION (GETADDRINFO / GETHOSTBYNAME)
        // ===================================================================
        case WSAHOST_NOT_FOUND:
            return "[NET_DNS_NOT_FOUND] Host not found. Name resolution failed.";
        case WSATRY_AGAIN:
            return "[NET_DNS_TRY_AGAIN] Temporary DNS failure. Retrying may succeed.";
        case WSANO_RECOVERY:
            return "[NET_DNS_NORECOVERY] Non-recoverable DNS database error.";
        case WSANO_DATA:
            return "[NET_DNS_NO_DATA] Valid host name, but no matching IP record found.";

        // ===================================================================
        // 9. EVENT & OVERLAPPED I/O ERRORS
        // ===================================================================
        case WSA_INVALID_HANDLE:
            return "[NET_EVT_BAD_HANDLE] Specified event object handle is invalid.";
        case WSA_INVALID_PARAMETER:
            return "[NET_EVT_BAD_PARAM] One or more parameters are invalid.";
        case WSA_IO_INCOMPLETE:
            return "[NET_EVT_INCOMPLETE] Overlapped I/O event object is not signaled.";
        case WSA_IO_PENDING:
            return "[NET_EVT_PENDING] Overlapped operations will complete later.";
        case WSA_OPERATION_ABORTED:
            return "[NET_EVT_ABORTED] Overlapped operation aborted due to socket closure.";

        // ===================================================================
        // FALLBACK
        // ===================================================================
        default:
            return "[NET_ERR_UNKNOWN] Unhandled Winsock error code: " + std::to_string(errorCode);
    }
}