#include "stdafx.hpp"

#include "net/address.hpp"

using namespace sm;
using namespace sm::net;

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

std::string net::toAddressString(const Address& addr) {
    if (addr.hasHostName())
        return addr.hostName();

    return fmtIp4Address(addr.v4address());
}

std::string net::toString(const Address& addr) {
    if (addr.hasHostName())
        return fmt::format("HostName({})", addr.hostName());

    return fmt::format("IPv4Address({})", fmtIp4Address(addr.v4address()));
}
