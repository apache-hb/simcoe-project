#pragma once

#include <string_view>
#include <chrono>

#include <WinSock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>

namespace sm::system::os {
    /// error code mapping
    using NetErrorCode = int;
    static constexpr NetErrorCode kSuccess = ERROR_SUCCESS;
    static constexpr NetErrorCode kErrorTimeout = ERROR_TIMEOUT;
    static constexpr NetErrorCode kErrorInterrupted = WSAEINTR;
    static constexpr NetErrorCode kNotInitialized = WSANOTINITIALISED;
    static constexpr NetErrorCode kWouldBlock = WSAEWOULDBLOCK;

    inline NetErrorCode lastNetError() {
        return ::WSAGetLastError();
    }

    /// setup data
    struct NetData {
        WSADATA data;

        bool isInitialized() { return data.wVersion != 0; }
        int version() { return data.wVersion; }
        int highVersion() { return data.wHighVersion; }

        std::string_view description() { return data.szDescription; }
        std::string_view systemStatus() { return data.szSystemStatus; }
    };

    inline NetErrorCode setupNetwork(NetData& data) {
        return ::WSAStartup(MAKEWORD(2, 2), &data.data);
    }

    inline NetErrorCode destroyNetwork(NetData& data) {
        return ::WSACleanup();
    }

    /// socket handle

    using SocketHandle = SOCKET;
    static constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;

    NetErrorCode destroySocket(SocketHandle socket);

    inline bool ioctlSocketAsync(SocketHandle socket, bool enabled) {
        u_long mode = enabled ? 0 : 1;
        return ::ioctlsocket(socket, FIONBIO, &mode) != SOCKET_ERROR;
    }

    inline bool cancelSocket(SocketHandle socket) {
        return ::shutdown(socket, SD_BOTH) == 0;
    }

    bool connectWithTimeout(SocketHandle socket, std::chrono::milliseconds timeout);
}
