#pragma once

#include <simcoe_config.h>

#include <limits>

#include "core/text.hpp"

#include "core.reflect.h"

namespace sm {
    struct Memory {
        // dont use reflection enums here, this does quite bespoke to_string behaviour
        enum Unit {
            eBytes,
            eKilobytes,
            eMegabytes,
            eGigabytes,
            eTerabytes,
            eLimit
        };

        static constexpr size_t kByte = 1;
        static constexpr size_t kKilobyte = kByte * 1024;
        static constexpr size_t kMegabyte = kKilobyte * 1024;
        static constexpr size_t kGigabyte = kMegabyte * 1024;
        static constexpr size_t kTerabyte = kGigabyte * 1024;

        static constexpr size_t kSizes[eLimit] = {
            kByte,
            kKilobyte,
            kMegabyte,
            kGigabyte,
            kTerabyte
        };

        static constexpr const char *kNames[eLimit] = {
            "b", "kb", "mb", "gb", "tb"
        };

        constexpr Memory(std::integral auto memory = 0, Unit unit = eBytes)
            : m_bytes(memory * kSizes[unit])
        { }

        constexpr size_t b() const { return m_bytes; }
        constexpr size_t kb() const { return m_bytes / kKilobyte; }
        constexpr size_t mb() const { return m_bytes / kMegabyte; }
        constexpr size_t gb() const { return m_bytes / kGigabyte; }
        constexpr size_t tb() const { return m_bytes / kTerabyte; }

        constexpr size_t as_bytes() const { return m_bytes; }
        constexpr size_t as_kilobytes() const { return m_bytes / kKilobyte; }
        constexpr size_t as_megabytes() const { return m_bytes / kMegabyte; }
        constexpr size_t as_gigabytes() const { return m_bytes / kGigabyte; }
        constexpr size_t as_terabytes() const { return m_bytes / kTerabyte; }

        friend constexpr auto operator<=>(const Memory& lhs, const Memory& rhs) = default;

        sm::String to_string() const;

    private:
        size_t m_bytes;
    };

    constexpr Memory bytes(size_t bytes) { return Memory(bytes, Memory::eBytes); }
    constexpr Memory kilobytes(size_t kilobytes) { return Memory(kilobytes, Memory::eKilobytes); }
    constexpr Memory megabytes(size_t megabytes) { return Memory(megabytes, Memory::eMegabytes); }
    constexpr Memory gigabytes(size_t gigabytes) { return Memory(gigabytes, Memory::eGigabytes); }
    constexpr Memory terabytes(size_t terabytes) { return Memory(terabytes, Memory::eTerabytes); }

    template<typename T, typename O>
    struct CastResult {
        static inline constexpr T kDstMin = std::numeric_limits<T>::min();
        static inline constexpr T kDstMax = std::numeric_limits<T>::max();

        static inline constexpr O kSrcMin = std::numeric_limits<O>::min();
        static inline constexpr O kSrcMax = std::numeric_limits<O>::max();

        T value;
        CastError error = CastError::eNone;
    };

    template<typename T, typename O>
    CastResult<T, O> checked_cast(O value) {
        using C = CastResult<T, O>;

        if constexpr (C::kDstMax > C::kSrcMax) {
            if (value < C::kSrcMin) {
                return { C::kDstMin, CastError::eUnderflow };
            }
        }

        if constexpr (C::kDstMin < C::kSrcMin) {
            if (value > C::kSrcMax) {
                return { C::kDstMax, CastError::eOverflow };
            }
        }

        return { static_cast<T>(value), CastError::eNone };
    }

    template<typename T, typename O>
    T int_cast(O value) {
        /* paranoia */
#if SMC_DEBUG
        static constexpr T kDstMin = std::numeric_limits<T>::min();
        static constexpr T kDstMax = std::numeric_limits<T>::max();

        static constexpr O kSrcMin = std::numeric_limits<O>::min();
        static constexpr O kSrcMax = std::numeric_limits<O>::max();

        if constexpr (kDstMax > kSrcMax) {
            CTASSERTF(value >= kSrcMin, "value %s would underflow (limit: %s)", value, kSrcMin);
        }

        if constexpr (kDstMin < kSrcMin) {
            CTASSERTF(value <= kSrcMax, "value %s would overflow (limit: %s)", value, kSrcMax);
        }
#endif

        return static_cast<T>(value);
    }

    template<typename T, typename O> requires std::is_enum_v<T> || std::is_enum_v<O>
    T enum_cast(O value) {
        if constexpr (std::is_enum_v<T>) {
            return T(int_cast<std::underlying_type_t<T>>(value));
        } else {
            return T(value);
        }
    }

    template<typename T>
    constexpr T roundup_pow2(T value) {
        T result = 1;
        while (result < value) {
            result <<= 1;
        }

        return result;
    }
}

#if SM_FORMAT
template<>
struct fmt::formatter<sm::Memory> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const sm::Memory& value, FormatContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", value.to_string());
    }
};
#endif
