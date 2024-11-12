#pragma once

#include <WinSock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

namespace sm::system::os {
    /// error code mapping
    using NetErrorCode = int;
    static constexpr NetErrorCode kSuccess = ERROR_SUCCESS;
    static constexpr NetErrorCode kErrorTimeout = ERROR_TIMEOUT;
    static constexpr NetErrorCode kErrorInterrupted = WSAEINTR;

    inline NetErrorCode lastNetError() {
        return ::WSAGetLastError();
    }

    /// address info

    using AddressInfo = addrinfo;

    inline NetErrorCode createAddressInfo(const char *node, const char *service, const AddressInfo *hints, AddressInfo **res) {
        return ::getaddrinfo(node, service, hints, res);
    }

    inline NetErrorCode destroyAddressInfo(AddressInfo *ai) {
        ::freeaddrinfo(ai);
        return kSuccess;
    }

    /// socket handle

    using SocketHandle = SOCKET;
    static constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;

    inline SocketHandle createSocket(int af, int type, int protocol) {
        return ::socket(af, type, protocol);
    }

    inline NetErrorCode destroySocket(SocketHandle socket) {
        return ::closesocket(socket);
    }

    inline NetErrorCode connectSocket(SocketHandle socket, const sockaddr *name, int namelen) {
        return ::connect(socket, name, namelen);
    }

    inline NetErrorCode bindSocket(SocketHandle socket, const sockaddr *name, int namelen) {
        return ::bind(socket, name, namelen);
    }

    inline NetErrorCode listenSocket(SocketHandle socket, int backlog) {
        return ::listen(socket, backlog);
    }
}
