#pragma once

#include <string_view>
#include <chrono>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

namespace sm::system::os {
    /// error code mapping
    using NetErrorCode = int;
    static constexpr NetErrorCode kSuccess = 0;
    static constexpr NetErrorCode kErrorTimeout = ETIMEDOUT;
    static constexpr NetErrorCode kErrorInterrupted = EINTR;
    static constexpr NetErrorCode kNotInitialized = ENOTCONN;
    static constexpr NetErrorCode kWouldBlock = EWOULDBLOCK;

    inline NetErrorCode lastNetError() {
        return errno;
    }

    /// setup data
    struct NetData {
        bool initialized = false;

        bool isInitialized() { return initialized; }
        int version() { return 1; }
        int highVersion() { return 0; }
        std::string_view description() { return "POSIX SOCKETS"; }
        std::string_view systemStatus() { return "ACTIVE"; }
    };

    inline NetErrorCode setupNetwork(NetData& data) {
        data.initialized = true;
        return kSuccess;
    }

    inline NetErrorCode destroyNetwork(NetData& data) {
        return kSuccess;
    }

    /// socket handle

    using SocketHandle = int;
    static constexpr SocketHandle kInvalidSocket = -1;

    NetErrorCode destroySocket(SocketHandle socket);

    inline bool ioctlSocketAsync(SocketHandle socket, bool enabled) {
        int flags = ::fcntl(socket, F_GETFL, 0);
        if (flags == -1)
            return false;

        int mode = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
        return ::fcntl(socket, F_SETFL, mode) != -1;
    }

    inline bool cancelSocket(SocketHandle socket) {
        return ::shutdown(socket, SHUT_RDWR) == 0;
    }

    bool connectWithTimeout(SocketHandle socket, const sockaddr *addr, socklen_t len, std::chrono::milliseconds timeout);
}
