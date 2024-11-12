#pragma once

#include <sys/socket.h>
#include <errno.h>

namespace sm::system::os {
    /// error code mapping
    using NetErrorCode = int;
    static constexpr NetErrorCode kSuccess = 0;
    static constexpr NetErrorCode kErrorTimeout = ETIMEDOUT;
    static constexpr NetErrorCode kErrorInterrupted = EINTR;

    inline NetErrorCode lastNetError() {
        return errno;
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

    using SocketHandle = int;
    static constexpr SocketHandle kInvalidSocket = -1;

    inline SocketHandle createSocket(int af, int type, int protocol) {
        return ::socket(af, type, protocol);
    }

    inline NetErrorCode destroySocket(SocketHandle socket) {
        return ::close(socket);
    }

    inline NetErrorCode connectSocket(SocketHandle socket, const sockaddr *name, socklen_t namelen) {
        return ::connect(socket, name, namelen);
    }
}
