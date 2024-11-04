#include "stdafx.hpp"

#include "common.hpp"

using namespace sm;
using namespace sm::net;

namespace chrono = std::chrono;

///
/// socket
///

void Socket::closeSocket() noexcept {
    closesocket(mSocket);
    mSocket = INVALID_SOCKET;
}

Socket::~Socket() noexcept {
    closeSocket();
}

NetResult<size_t> Socket::sendBytes(const void *data, size_t size) noexcept {
    int sent = ::send(mSocket, static_cast<const char *>(data), size, 0);
    if (sent == SOCKET_ERROR)
        return std::unexpected(lastNetError());

    return sent;
}

NetResult<size_t> Socket::recvBytes(void *data, size_t size) noexcept {
    int received = ::recv(mSocket, static_cast<char *>(data), size, 0);
    if (received == SOCKET_ERROR)
        return std::unexpected(lastNetError());

    if (received == 0)
        return std::unexpected(NetError{SNET_CONNECTION_CLOSED});

    return received;
}

ReadResult Socket::recvBytesTimeout(void *data, size_t size, std::chrono::milliseconds timeout) noexcept {
    const chrono::time_point start = chrono::steady_clock::now();
    size_t consumed = 0;

    auto timeoutReached = [&] { return start + timeout < chrono::steady_clock::now(); };

    while (!timeoutReached() && consumed < size) {
        char *ptr = static_cast<char *>(data) + consumed;
        int remaining = size - consumed;

        int received = ::recv(mSocket, ptr, remaining, 0);
        if (received > 0) {
            consumed += received;

            if (consumed >= size)
                return { consumed, NetError::ok() };

        } else {
            int lastError = WSAGetLastError();
            if (lastError != WSAEWOULDBLOCK)
                return { consumed, NetError{lastError} };

        }
    }

    return { consumed, NetError{ERROR_TIMEOUT} };
}

NetError Socket::setBlocking(bool blocking) noexcept {
    u_long mode = blocking ? 0 : 1;
    if (ioctlsocket(mSocket, FIONBIO, &mode))
        return lastNetError();

    mBlocking = blocking;

    return NetError::ok();
}

NetError Socket::setRecvTimeout(std::chrono::milliseconds timeout) noexcept {
    auto count = timeout.count();
    struct timeval tv = {
        .tv_sec = static_cast<long>(count / 1000),
        .tv_usec = static_cast<long>((count % 1000))
    };

    if (setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv)))
        return lastNetError();

    return NetError::ok();
}

NetError Socket::setSendTimeout(std::chrono::milliseconds timeout) noexcept {
    auto count = timeout.count();
    struct timeval tv = {
        .tv_sec = static_cast<long>(count / 1000),
        .tv_usec = static_cast<long>((count % 1000))
    };

    if (setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv)))
        return lastNetError();

    return NetError::ok();
}


///
/// listen socket
///

ListenSocket::~ListenSocket() noexcept {
    cancel();
}

NetResult<Socket> ListenSocket::tryAccept() noexcept {
    SOCKET client = ::accept(mSocket, nullptr, nullptr);
    if (client == INVALID_SOCKET)
        return std::unexpected(lastNetError());

    return Socket{client};
}

void ListenSocket::cancel() noexcept {
    shutdown(mSocket, SD_BOTH);
    Socket::closeSocket();
}

NetError ListenSocket::listen(int backlog) noexcept {
    if (::listen(mSocket, backlog))
        return lastNetError();

    return NetError{0};
}
