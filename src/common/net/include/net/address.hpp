#pragma once

#include <array>
#include <span>
#include <string>

namespace sm::net {
    enum class AddressType {
        eIPv4,
        eIPv6,
    };

    class Address {
    public:
        static constexpr inline size_t kIpv4Size = 4;

        using IPv4Bytes = std::span<const uint8_t, kIpv4Size>;
        using IPv4Data = std::array<uint8_t, kIpv4Size>;

        static constexpr Address ipv4(uint8_t a, uint8_t b, uint8_t c, uint8_t d) noexcept {
            return IPv4Data {a, b, c, d};
        }

        IPv4Bytes v4address() const noexcept { return mIpv4Address; }
        bool hasHostName() const noexcept { return !mHostName.empty(); }
        std::string hostName() const noexcept { return mHostName; }

        static constexpr Address loopback() noexcept {
            return ipv4(127, 0, 0, 1);
        }

        static constexpr Address any() noexcept {
            return ipv4(0, 0, 0, 0);
        }

        static constexpr Address of(std::string host) noexcept {
            return Address{std::move(host)};
        }

    private:
        IPv4Data mIpv4Address;
        std::string mHostName;

        constexpr Address(IPv4Data data) noexcept
            : mIpv4Address(data)
        { }

        constexpr Address(std::string host) noexcept
            : mHostName(std::move(host))
        { }
    };

    std::string toAddressString(const Address& addr);
    std::string toString(const Address& addr);
}
