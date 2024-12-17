#pragma once

#include "base/panic.h"

#include "base/endian.hpp"

#include <stdint.h>
#include <string.h>

#include <chrono>
#include <functional>

namespace sm {
    namespace detail {
        using namespace std::chrono_literals;

        static constexpr std::chrono::system_clock::time_point kGregorianReform = std::chrono::sys_days{1582y/10/15};

        class rfc9562_clock {
        public:
            // 100ns intervals since 1582-10-15T00:00:00Z
            using duration = std::chrono::duration<uint64_t, std::ratio_multiply<std::nano, std::ratio<100>>>;
            using rep = duration::rep;
            using period = duration::period;
            using time_point = std::chrono::time_point<rfc9562_clock>;

            static time_point from_sys(std::chrono::system_clock::time_point time);
            static std::chrono::system_clock::time_point to_sys(time_point time);
        };
    }

    struct MacAddress {
        uint8_t octets[6];

        constexpr MacAddress() = default;
        constexpr MacAddress(uint64_t value) noexcept
            : octets {
                static_cast<uint8_t>(value >> 40),
                static_cast<uint8_t>(value >> 32),
                static_cast<uint8_t>(value >> 24),
                static_cast<uint8_t>(value >> 16),
                static_cast<uint8_t>(value >> 8),
                static_cast<uint8_t>(value),
            }
        { }

        uint32_t organizationId() const noexcept {
            return (octets[0] << 16) | (octets[1] << 8) | octets[2];
        }

        uint32_t networkId() const noexcept {
            return (octets[3] << 16) | (octets[4] << 8) | octets[5];
        }

        bool multicast() const noexcept {
            return (octets[0] & 0b1);
        }

        bool unicast() const noexcept {
            return !(octets[0] & 0b1);
        }

        bool localUnique() const noexcept {
            return (octets[0] & 0b10);
        }

        bool globalUnique() const noexcept {
            return !(octets[0] & 0b10);
        }

        constexpr bool operator==(const MacAddress& other) const noexcept = default;
    };

    // https://www.rfc-editor.org/rfc/rfc9562.html#name-uuid-version-1
    struct uuidv1 {
        le<uint32_t> time0;
        le<uint16_t> time1;
        be<uint16_t> time2; // bits 48-51 are version
        be<uint16_t> clockSeq; // top 2 bits are variant
        MacAddress node;
    };

    // X/Open CAE Specification A.1 (Universal Unique Identifier)
    // https://pubs.opengroup.org/onlinepubs/9696999099/toc.pdf
    // DCE security UUID with embedded POSIX UID
    struct uuidv2 {
        // unimplemented for now, i cant find the actual spec for this :(
    };

    // https://www.rfc-editor.org/rfc/rfc9562.html#name-uuid-version-3
    struct uuidv3 {
        uint8_t md5[16];
    };

    struct uuidv6 {
        be<uint32_t> time2;
        be<uint16_t> time1;
        be<uint16_t> time0;
        be<uint16_t> clockSeq;
        MacAddress node;
    };

    struct uuidv7 {
        be<uint32_t> time0;
        be<uint16_t> time1;
        uint8_t rand[10];
    };

    struct uuid {
        /// @brief Total characters required to represent a uuid in 8-4-4-4-12 format.
        /// @note Not including nul terminator.
        static constexpr size_t kStringSize = 36;

        /// @brief Total characters required to represent a microsoft uuid in {8-4-4-4-12} format.
        /// @note Not including nul terminator.
        static constexpr size_t kMicrosoftStringSize = kStringSize + 2;

        /// @brief Total characters required to represent a uuid in raw hex format.
        /// @note Not including nul terminator.
        static constexpr size_t kHexStringSize = 32;

        /// @brief The maximum size required to represent a uuid in any format.
        /// @note Not including nul terminator.
        static constexpr size_t kMaxStringSize = std::max({ kStringSize, kMicrosoftStringSize, kHexStringSize });

        enum Version {
            eVersion1 = 0b0001,
            eVersion6 = 0b0110,
            eVersion7 = 0b0111,
        };

        enum Variant {
            eReserved  = 0b0,
            eDCE       = 0b10,
            eMicrosoft = 0b110,
            eFuture    = 0b1110,
        };

        union {
            uint8_t octets[16];
            uuidv1 uv1;
            uuidv3 uv3;
            uuidv6 uv6;
            uuidv7 uv7;
        };

        // factory functions

        static constexpr uuid nil() noexcept {
            return uuid {
                {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}},
            };
        }

        static constexpr uuid max() noexcept {
            return uuid {
                {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
            };
        }

        static constexpr uuid of(be<uint32_t> a, be<uint16_t> b, be<uint16_t> c, be<uint16_t> d, be<uint64_t> e) noexcept {
            e = e << 16;
            uuid result;
            memcpy(result.octets, &a, sizeof(a));
            memcpy(result.octets + 4, &b, sizeof(b));
            memcpy(result.octets + 6, &c, sizeof(c));
            memcpy(result.octets + 8, &d, sizeof(d));
            memcpy(result.octets + 10, &e, 6);
            return result;
        }

        constexpr uint8_t version() const noexcept {
            // https://www.rfc-editor.org/rfc/rfc9562.html#version_field
            // version info is located in bits 48-51 in octet 6
            return (octets[6] & 0xF0) >> 4;
        }

        constexpr void setVersion(uint8_t version) noexcept {
            octets[6] = (octets[6] & 0x0F) | (version << 4);
        }

        constexpr uint8_t variant() const noexcept {
            // https://www.rfc-editor.org/rfc/rfc9562.html#name-variant-field
            uint8_t variant = octets[8];
            if ((variant & 0b1000'0000) == 0) {
                return variant >> 4; // 0b0xxx indicates the reserved range 1-7
            }

            if ((variant & 0b1100'0000) == 0b1000'0000) {
                return (variant & 0b1100'0000) >> 6; // 0b10xx indicates an rfc9562 uuid
            }

            if ((variant & 0b1110'0000) == 0b1100'0000) {
                return (variant & 0b1110'0000) >> 4; // 0b110x is reserved for microsoft
            }

            return variant >> 4; // all remaining variants are reserved for future use
        }

        constexpr void setVariant(uint8_t var) noexcept {
            // TODO: for some versions this may not be correct
            // but for everything specified by rfc9562 this is fine
            octets[8] = (octets[8] & ((var << 6) * 2 + 1)) | (var << 6);
        }

        constexpr bool operator==(const uuid& other) const noexcept {
            return memcmp(octets, other.octets, sizeof(octets)) == 0;
        }

        // v1 uuid api

        static uuid v1(std::chrono::utc_clock::time_point time, uint16_t clockSeq, MacAddress node);

        uint16_t v1ClockSeq() const noexcept;
        MacAddress v1Node() const noexcept;

        std::chrono::utc_clock::time_point v1Time() const noexcept;

        // v3 uuid api

        static uuid v3(const uint8_t md5[16]);

        void v3Md5Hash(uint8_t md5[16]) const noexcept;

        // v6 uuid api

        static uuid v6(std::chrono::utc_clock::time_point time, uint16_t clockSeq, MacAddress node);

        uint16_t v6ClockSeq() const noexcept;
        MacAddress v6Node() const noexcept;

        std::chrono::utc_clock::time_point v6Time() const noexcept;

        // v7 uuid api

        static uuid v7(std::chrono::system_clock::time_point time, const uint8_t random[10]);

        std::chrono::system_clock::time_point v7Time() const noexcept;

        /// @brief Convert the uuid to a string
        /// Converts the uuid to a string in the format 8-4-4-4-12
        /// @param dst the buffer to write the string to
        /// @note The buffer must be at least kStringSize + 1 bytes long
        static void strfuid(char dst[kStringSize], uuid uuid) noexcept;

        /// @brief parse a uuid from a string
        /// Only supports 8-4-4-4-12 hex format with hyphens, use @a parseMicrosoft for microsoft format
        /// or @a parseHex for raw hex format. On success the result will be written to @a result.
        /// On failure will return false and @a result will be unchanged.
        /// @param str the string to parse
        /// @param result the uuid to write the result to
        /// @return true if the string was successfully parsed
        static bool parse(const char str[kStringSize], uuid& result) noexcept;

        /// @brief parse a microsoft uuid from a string
        /// Only supports {8-4-4-4-12} format with braces, use @a parse for standard format
        /// or @a parseHex for raw hex format. On success the result will be written to @a result.
        /// On failure will return false and @a result will be unchanged.
        /// @param str the string to parse
        /// @param result the uuid to write the result to
        /// @return true if the string was successfully parsed
        static bool parseMicrosoft(const char str[kMicrosoftStringSize], uuid& result) noexcept;

        /// @brief parse a uuid from a raw hex string
        /// Only supports 32 character hex strings. On success the result will be written to @a result.
        /// On failure will return false and @a result will be unchanged.
        /// @param str the string to parse
        /// @param result the uuid to write the result to
        /// @return true if the string was successfully parsed
        static bool parseHex(const char str[kHexStringSize], uuid& result) noexcept;

        /// @brief parse a uuid from any format
        /// Attempts to parse the uuid from any format. On success the result will be written to @a result.
        /// On failure will return false and @a result will be unchanged.
        /// @param str the string to parse
        /// @param result the uuid to write the result to
        /// @return true if the string was successfully parsed
        static bool parseAny(const char str[kMaxStringSize], uuid& result) noexcept;
    };

    static_assert(sizeof(uuidv1) == 16);
    static_assert(sizeof(uuidv3) == 16);
    static_assert(sizeof(uuidv6) == 16);
    static_assert(sizeof(uuidv7) == 16);
    static_assert(sizeof(uuid) == 16);
}

template<>
struct std::hash<sm::uuid> {
    size_t operator()(const sm::uuid& guid) const noexcept {
        size_t hash = 0;
        for (uint8_t byte : guid.octets) {
            hash = hash * 31 + byte;
        }

        return hash;
    }
};
