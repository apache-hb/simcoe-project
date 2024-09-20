#include "stdafx.hpp"

#include "core/error.hpp"

#include "core/defer.hpp"

#include "logs/structured/message.hpp"

#include "net/net.hpp"

using namespace sm;
using namespace sm::net;

static NetError lastNetError() noexcept {
    return NetError{WSAGetLastError()};
}

std::string net::toString(const IPv4Address& addr) noexcept {
    const uint8_t *bytes = addr.address();
    return fmt::format("{}.{}.{}.{}", bytes[0], bytes[1], bytes[2], bytes[3]);
}

NetError::NetError(int code) noexcept
    : mCode(code)
    , mMessage(fmt::format("{} ({})", OsError(code), code))
{ }

void NetError::raise() const noexcept(false) {
    throw NetException{*this};
}

void NetError::throwIfFailed() const noexcept(false) {
    if (mCode != 0)
        raise();
}

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

    return received;
}

///
/// listen socket
///

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

///
/// network
///

void Network::cleanup() noexcept {
    int result = WSACleanup();
    if (result != 0) {
        LOG_ERROR("WSACleanup failed with error: {}", result);
    }

    mData = WSADATA{};
}

Network::~Network() noexcept {
    if (mData.wVersion != 0)
        cleanup();
}

NetResult<Network> Network::tryCreate() noexcept {
    WSADATA data{};
    if (int result = WSAStartup(MAKEWORD(2, 2), &data))
        return std::unexpected(NetError{result});

    LOG_INFO("WSAStartup successful. version {}.{}", data.wVersion, data.wHighVersion);

    LOG_INFO("Description: `{}`, Status: `{}`",
        std::string{data.szDescription},
        std::string{data.szSystemStatus}
    );

    return Network{data};
}

static SOCKET findOpenSocket(addrinfo *info) noexcept {
    for (addrinfo *ptr = info; ptr != nullptr; ptr = ptr->ai_next) {
        SOCKET socket = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket == INVALID_SOCKET)
            return socket;

        int err = ::connect(socket, ptr->ai_addr, ptr->ai_addrlen);
        if (err != SOCKET_ERROR)
            return socket;

        closesocket(socket);
    }

    return INVALID_SOCKET;
}

NetResult<Socket> Network::tryConnect(IPv4Address address, uint16_t port) noexcept {
    addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
    };

    addrinfo *result = nullptr;

    std::string addr = toString(address);
    std::string p = std::to_string(port);
    if (int err = getaddrinfo(addr.c_str(), p.c_str(), &hints, &result)) {
        return std::unexpected(NetError{err});
    }

    SOCKET socket = findOpenSocket(result);

    freeaddrinfo(result);

    if (socket == INVALID_SOCKET)
        return std::unexpected(lastNetError());

    return Socket{socket};
}

NetResult<ListenSocket> Network::tryBind(IPv4Address address, uint16_t port) noexcept {
    addrinfo hints = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
    };

    addrinfo *result = nullptr;

    std::string addr = toString(address);
    std::string p = std::to_string(port);

    if (int err = getaddrinfo(addr.c_str(), p.c_str(), &hints, &result)) {
        return std::unexpected(NetError(err));
    }

    defer { freeaddrinfo(result); };

    SOCKET socket = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket == INVALID_SOCKET)
        return std::unexpected(lastNetError());

    if (::bind(socket, result->ai_addr, result->ai_addrlen)) {
        closesocket(socket);
        return std::unexpected(lastNetError());
    }

    return ListenSocket{socket};
}
