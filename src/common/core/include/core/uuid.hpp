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

        static constexpr std::chrono::system_clock::duration distance(std::chrono::system_clock::time_point from, std::chrono::system_clock::time_point to) {
            auto begin = from.time_since_epoch();
            auto end = to.time_since_epoch();

            return (begin > end) ? begin - end : end - begin;
        }

        class rfc9562_clock {
        public:
            // 100ns intervals since 1582-10-15T00:00:00Z
            using duration = std::chrono::duration<uint64_t, std::ratio_multiply<std::nano, std::ratio<100>>>;
            using rep = duration::rep;
            using period = duration::period;
            using time_point = std::chrono::time_point<rfc9562_clock>;

            static time_point from_sys(std::chrono::system_clock::time_point time) {
                // our epoch is 1582-10-15T00:00:00Z, the gregorian reform
                // we need to convert from the system clock epoch to the rfc9562 epoch.

                return rfc9562_clock::time_point{distance(kGregorianReform, time)};
            }

            static std::chrono::system_clock::time_point to_sys(time_point time) {
                return std::chrono::system_clock::time_point{std::chrono::duration_cast<std::chrono::system_clock::duration>((kGregorianReform.time_since_epoch() + time.time_since_epoch()))};
            }
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
            return (octets[0] & 0x01) == 0x01;
        }

        bool unicast() const noexcept {
            return !multicast();
        }

        bool localUnique() const noexcept {
            return (octets[0] & 0x02) == 0x02;
        }

        bool globalUnique() const noexcept {
            return !localUnique();
        }

        constexpr bool operator==(const MacAddress& other) const noexcept = default;
    };

    // https://www.rfc-editor.org/rfc/rfc9562.html#name-uuid-version-1
    struct uuidv1 {
        be<uint32_t> time0;
        be<uint16_t> time1;
        be<uint16_t> time2; // bits 48-51 are version
        be<uint16_t> clockSeq; // top 2 bits are variant
        MacAddress node;
    };

    struct uuidfields {
        be<uint32_t> f0;
        be<uint16_t> f1;
        be<uint16_t> f2;
        be<uint16_t> f3;
        be<uint16_t> f4;
        be<uint32_t> f5;
    };

    struct uuid {
        /// @brief Total characters required to represent a uuid in 8-4-4-4-12 format.
        /// @note Not including nul terminator.
        static constexpr size_t kStringSize = 36;

        enum Version {
            eVersion1 = 0b0001
        };

        enum Variant {
            eReserved  = 0b0,
            eRfc9562   = 0b10,
            eMicrosoft = 0b110,
            eFuture    = 0b1110,
        };

        union {
            uint8_t octets[16];
            uuidv1 uv1;
            uuidfields ufs;
        };

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

        /// @brief Convert the uuid to a string
        /// Converts the uuid to a string in the format 8-4-4-4-12
        /// @param dst the buffer to write the string to
        /// @note The buffer must be at least kStringSize + 1 bytes long
        constexpr static void strfuid(char dst[kStringSize], uuid uuid) noexcept {
            auto writeOctets = [&](size_t start, size_t n, size_t o) {
                constexpr char kHex[] = "0123456789abcdef";
                for (size_t i = start; i < n; i++) {
                    dst[i * 2 + o + 0] = kHex[uuid.octets[i] >> 4];
                    dst[i * 2 + o + 1] = kHex[uuid.octets[i] & 0x0F];
                }
            };

            writeOctets(0, 4, 0);
            dst[8] = '-';
            writeOctets(4, 6, 1);
            dst[13] = '-';
            writeOctets(6, 8, 2);
            dst[18] = '-';
            writeOctets(8, 10, 3);
            dst[23] = '-';
            writeOctets(10, 16, 4);
        }
    };

    static_assert(sizeof(uuidv1) == 16);
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
