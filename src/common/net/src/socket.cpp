#include "stdafx.hpp"

#include "common.hpp"

#include <cstdio>

using namespace sm;
using namespace sm::net;

namespace chrono = std::chrono;

///
/// socket
///

static void closeSocket(std::atomic<system::os::SocketHandle> *socket) {
    if (socket != nullptr) {
        system::os::SocketHandle handle = socket->exchange(system::os::kInvalidSocket);
        if (handle != system::os::kInvalidSocket) {
            system::os::destroySocket(handle);
        }
    }
}

Socket::~Socket() noexcept {
    closeSocket(mSocket.get());
}

NetResult<size_t> Socket::sendBytes(const void *data, size_t size) noexcept {
    int sent = ::send(get(), static_cast<const char *>(data), size, 0);
    if (sent == -1)
        return std::unexpected(lastNetError());

    return sent;
}

NetResult<size_t> Socket::recvBytes(void *data, size_t size) noexcept {
    int received = ::recv(get(), static_cast<char *>(data), size, 0);
    if (received == -1)
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

        int received = ::recv(get(), ptr, remaining, 0);
        if (received > 0) {
            consumed += received;

            if (consumed >= size)
                return { consumed, NetError::ok() };

        } else {
            int lastError = system::os::lastNetError();
            if (lastError != system::os::kWouldBlock)
                return { consumed, NetError{lastError} };

        }
    }

    return { consumed, NetError{system::os::kErrorTimeout} };
}

NetError Socket::setBlocking(bool blocking) noexcept {
    if (!system::os::ioctlSocketAsync(get(), blocking))
        return lastNetError();

    if (blocking) {
        mFlags->fetch_or(kBlockingFlag);
    } else {
        mFlags->fetch_and(~kBlockingFlag);
    }

    return NetError::ok();
}

bool Socket::isActive() const noexcept {
    system::os::SocketHandle handle = mSocket ? mSocket->load() : system::os::kInvalidSocket;
    if (handle == system::os::kInvalidSocket)
        return false;

    int error = 0;
    socklen_t len = sizeof(error);
    if (::getsockopt(handle, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &len) == -1)
        return false;

    return error == 0;
}

NetError Socket::setRecvTimeout(std::chrono::milliseconds timeout) noexcept {
    auto count = timeout.count();
    struct timeval tv = {
        .tv_sec = static_cast<long>(count / 1000),
        .tv_usec = static_cast<long>(count % 1000)
    };

    if (::setsockopt(get(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv)))
        return lastNetError();

    return NetError::ok();
}

NetError Socket::setSendTimeout(std::chrono::milliseconds timeout) noexcept {
    auto count = timeout.count();
    struct timeval tv = {
        .tv_sec = static_cast<long>(count / 1000),
        .tv_usec = static_cast<long>(count % 1000)
    };

    if (::setsockopt(get(), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv)))
        return lastNetError();

    return NetError::ok();
}


///
/// listen socket
///

NetResult<Socket> ListenSocket::tryAccept() noexcept {
    system::os::SocketHandle client = ::accept(get(), nullptr, nullptr);
    if (client == system::os::kInvalidSocket) {

        // if we were shutdown then return an error
        if (!isActive())
            return std::unexpected(NetError{system::os::kErrorInterrupted});

        return std::unexpected(lastNetError());
    }

    return {client};
}

Socket ListenSocket::accept() noexcept(false) {
    return throwIfFailed(tryAccept());
}

void ListenSocket::cancel() noexcept {
    closeSocket(mSocket.get());
}

NetError ListenSocket::listen(int backlog) noexcept {
    if (::listen(get(), backlog)) {
        if (!isActive())
            return NetError{system::os::kErrorInterrupted};

        return lastNetError();
    }

    return NetError::ok();
}

uint16_t ListenSocket::getBoundPort() {
    sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    if (::getsockname(get(), reinterpret_cast<sockaddr*>(&addr), &len))
        throw NetException{lastNetError()};

    if (addr.ss_family == AF_INET) {
        sockaddr_in *in = reinterpret_cast<sockaddr_in*>(&addr);
        return ntohs(in->sin_port);
    }

    return 0;
}
