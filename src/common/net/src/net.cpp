#include "stdafx.hpp"

#include "common.hpp"
#include "core/error.hpp"

using namespace sm;
using namespace sm::net;

namespace chrono = std::chrono;

static system::os::NetData gNetData{};

static bool isNetSetup() noexcept {
    return gNetData.isInitialized();
}

NetError lastNetError() {
    return NetError{system::os::lastNetError()};
}

///
/// network
///

void net::create(void) {
    CTASSERTF(!isNetSetup(), "network already initialized");

    if (int result = system::os::setupNetwork(gNetData))
        throw NetException{result};

    LOG_INFO(NetLog, "WSAStartup successful. {}.{}", gNetData.version(), gNetData.highVersion());

    LOG_INFO(NetLog, "Description: `{}`, Status: `{}`", gNetData.description(), gNetData.systemStatus());
}

void net::destroy(void) noexcept {
    if (!isNetSetup())
        return;

    int result = system::os::destroyNetwork(gNetData);
    if (result != 0) {
        LOG_ERROR(NetLog, "WSACleanup failed with error: {}", result);
    }

    gNetData = system::os::NetData{};
}

Network Network::create() noexcept(false) {
    if (!isNetSetup())
        throw NetException{system::os::kNotInitialized};

    return Network{};
}

static system::os::SocketHandle findOpenSocket(addrinfo *info) noexcept(false) {
    for (addrinfo *ptr = info; ptr != nullptr; ptr = ptr->ai_next) {

        // if we fail to even create a socket then throw an exception
        system::os::SocketHandle socket = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket == system::os::kInvalidSocket)
            throw NetException{lastNetError()};

        // failing to connect is acceptable, we can try the next address
        int err = ::connect(socket, ptr->ai_addr, ptr->ai_addrlen);
        if (err != system::os::kInvalidSocket)
            return socket;

        system::os::destroySocket(socket);
    }

    // if we reach this point then we failed to connect to any address
    throw NetException{SNET_CONNECTION_FAILED};
}

static bool socketIsWritable(system::os::SocketHandle socket) {
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(socket, &writefds);

    timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
    int result = select(0, nullptr, &writefds, nullptr, &timeout);
    return result > 0;
}

static system::os::SocketHandle findOpenSocketWithTimeout(addrinfo *info, std::chrono::milliseconds timeout) noexcept(false) {
    for (addrinfo *ptr = info; ptr != nullptr; ptr = ptr->ai_next) {

        // if we fail to even create a socket then throw an exception
        system::os::SocketHandle socket = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket == system::os::kInvalidSocket)
            throw NetException{lastNetError()};

        if (!system::os::ioctlSocketAsync(socket, true)) {
            NetError err = lastNetError();
            system::os::destroySocket(socket);
            throw NetException{err};
        }

        const chrono::time_point start = chrono::steady_clock::now();
        auto timeoutReached = [&] { return start + timeout < chrono::steady_clock::now(); };

        while (!timeoutReached()) {
            int err = ::connect(socket, ptr->ai_addr, ptr->ai_addrlen);
            if (err == 0)
                return socket;

            if (socketIsWritable(socket))
                return socket;
        }

        system::os::destroySocket(socket);
    }

    // if we reach this point then we failed to connect to any address
    throw NetException{system::os::kErrorTimeout};
}

static addrinfo *getAddrInfo(const Address& address, uint16_t port) throws(NetException) {
    addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
    };

    addrinfo *result = nullptr;

    std::string addr = toAddressString(address);
    std::string p = std::to_string(port);
    if (int err = ::getaddrinfo(addr.c_str(), p.c_str(), &hints, &result)) {
        throw NetException{err, "getaddrinfo({}:{})", addr, p};
    }

    return result;
}

Socket Network::connect(const Address& address, uint16_t port) noexcept(false) {
    addrinfo *result = getAddrInfo(address, port);

    defer { ::freeaddrinfo(result); };

    return Socket{findOpenSocket(result)};
}

Socket Network::connectWithTimeout(const Address& address, uint16_t port, std::chrono::milliseconds timeout) noexcept(false) {
    addrinfo *result = getAddrInfo(address, port);

    defer { ::freeaddrinfo(result); };

    return Socket{findOpenSocketWithTimeout(result, timeout)};
}

ListenSocket Network::bind(const Address& address, uint16_t port) noexcept(false) {
    addrinfo hints = {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
    };

    addrinfo *result = nullptr;

    std::string addr = toAddressString(address);
    std::string p = std::to_string(port);

    if (int err = getaddrinfo(addr.c_str(), p.c_str(), &hints, &result)) {
        throw NetException{NetError(err, "getaddrinfo({}:{})", addr, p)};
    }

    defer { ::freeaddrinfo(result); };

    system::os::SocketHandle socket = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (socket == system::os::kInvalidSocket) {
        throw NetException{lastNetError()};
    }

    if (::bind(socket, result->ai_addr, result->ai_addrlen)) {
        NetError error = lastNetError();
        system::os::destroySocket(socket);
        throw NetException{error};
    }

    return ListenSocket{socket};
}
