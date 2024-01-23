#pragma once

#include <string>

#include <limits>

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

        constexpr Memory(size_t memory = 0, Unit unit = eBytes)
            : m_bytes(memory * kSizes[unit])
        { }

        constexpr size_t b() const { return m_bytes; }
        constexpr size_t kb() const { return m_bytes / kKilobyte; }
        constexpr size_t mb() const { return m_bytes / kMegabyte; }
        constexpr size_t gb() const { return m_bytes / kGigabyte; }
        constexpr size_t tb() const { return m_bytes / kTerabyte; }

        std::string to_string() const;

    private:
        size_t m_bytes;
    };

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
            SM_ASSERTF(value >= kSrcMin, "value {} would underflow (limit: {})", value, kSrcMin);
        }

        if constexpr (kDstMin < kSrcMin) {
            SM_ASSERTF(value <= kSrcMax, "value {} would overflow (limit: {})", value, kSrcMax);
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
