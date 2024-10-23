#include "stdafx.hpp"

#include "common.hpp"
#include "core/error.hpp"

using namespace sm;
using namespace sm::net;

namespace chrono = std::chrono;

static WSADATA gNetData{};

static bool isNetSetup() noexcept {
    return gNetData.wVersion != 0;
}

static NetError lastNetError() {
    return NetError{WSAGetLastError()};
}

static std::string fmtIp4Address(Address::IPv4Bytes bytes) {
    return fmt::format("{}.{}.{}.{}", bytes[0], bytes[1], bytes[2], bytes[3]);
}

#if 0
static std::string fmtIp6Address(Address::IPv6Bytes bytes) {
    std::string buffer{INET6_ADDRSTRLEN};
    size_t index = 0;
    for (size_t i = 0; i < bytes.size(); i += 2) {
        if (i > 0)
            buffer[index++] = ':';

        uint8_t byte = bytes[i];
        if (byte != 0) {
            fmt::format_to_n(buffer.data() + index, buffer.size() - index, "{:02x}", byte);
        }
    }

    return buffer;
}
#endif

std::string net::toAddressString(Address addr) {
    return fmtIp4Address(addr.v4address());
}

std::string net::toString(Address addr) {
    return fmt::format("IPv4Address({})", fmtIp4Address(addr.v4address()));
}

static std::string fmtOsError(int code) {
    switch (code) {
    case SNET_READ_TIMEOUT:
        return "Read timeout (" CT_STR(SNET_READ_TIMEOUT) ")";
    case SNET_END_OF_PACKET:
        return "End of packet (" CT_STR(SNET_END_OF_PACKET) ")";
    case SNET_CONNECTION_CLOSED:
        return "Connection closed (" CT_STR(SNET_CONNECTION_CLOSED) ")";
    default:
        return fmt::format("OS error: {} ({})", OsError(code), code);
    }
}

NetError::NetError(int code)
    : Super(fmtOsError(code))
    , mCode(code)
{ }

///
/// network
///

void net::create(void) {
    if (isNetSetup()) {
        CT_NEVER("network already initialized");
    }

    if (int result = WSAStartup(MAKEWORD(2, 2), &gNetData))
        throw NetException{result};

    LOG_INFO(NetLog, "WSAStartup successful. {}.{}", gNetData.wVersion, gNetData.wHighVersion);

    LOG_INFO(NetLog, "Description: `{}`, Status: `{}`",
        std::string_view{gNetData.szDescription},
        std::string_view{gNetData.szSystemStatus}
    );
}

void net::destroy(void) noexcept {
    if (!isNetSetup())
        return;

    int result = WSACleanup();
    if (result != 0) {
        LOG_ERROR(NetLog, "WSACleanup failed with error: {error}", result);
    }

    gNetData = WSADATA{};
}

NetResult<Network> Network::tryCreate() noexcept {
    if (!isNetSetup())
        return std::unexpected(NetError{WSANOTINITIALISED});

    return Network{};
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

NetResult<Socket> Network::tryConnect(Address address, uint16_t port) noexcept {
    addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
    };

    addrinfo *result = nullptr;

    std::string addr = toAddressString(address);
    std::string p = std::to_string(port);
    if (int err = getaddrinfo(addr.c_str(), p.c_str(), &hints, &result)) {
        return std::unexpected(NetError(err, "getaddrinfo({}:{})", addr, p));
    }

    SOCKET socket = findOpenSocket(result);

    freeaddrinfo(result);

    if (socket == INVALID_SOCKET)
        return std::unexpected(lastNetError());

    return Socket{socket};
}

NetResult<ListenSocket> Network::tryBind(Address address, uint16_t port) noexcept {
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
        return std::unexpected(NetError(err, "getaddrinfo({}:{})", addr, p));
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

size_t Network::getMaxSockets() const noexcept {
    return gNetData.iMaxSockets;
}
