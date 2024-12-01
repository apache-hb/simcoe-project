#pragma once

#include <simcoe_config.h>

#include "core/traits.hpp"

#include <limits>

namespace sm {
    enum class CastError {
        eNone,
        eUnderflow,
        eOverflow
    };

    template<typename T, typename O>
    struct CastResult {

        using From = O;
        using To = T;

        static constexpr T dst_min() { return (std::numeric_limits<T>::min)(); }
        static constexpr T dst_max() { return (std::numeric_limits<T>::max)(); }

        static constexpr O src_min() { return (std::numeric_limits<O>::min)(); }
        static constexpr O src_max() { return (std::numeric_limits<O>::max)(); }

        T value;
        CastError error = CastError::eNone;
    };

    template<typename T, typename O>
    CastResult<T, O> checked_cast(O value) noexcept {
        using C = CastResult<T, O>;

        if constexpr (C::dst_max() > C::src_max()) {
            if (value < C::src_min()) {
                return { C::dst_min(), CastError::eUnderflow };
            }
        }

        if constexpr (C::dst_min() < C::src_min()) {
            if (value > C::src_max()) {
                return { C::dst_max(), CastError::eOverflow };
            }
        }

        return { static_cast<T>(value), CastError::eNone };
    }

    template<typename T, typename O>
    constexpr T int_cast(O value) noexcept {
#if SMC_DEBUG
        using C = CastResult<T, O>;

        constexpr O kSrcMin = C::src_min();
        constexpr O kSrcMax = C::src_max();

        if constexpr (C::dst_max() > kSrcMax) {
            SM_ASSERTF(value >= kSrcMin, "value {} would underflow (limit: {})", value, kSrcMin);
        }

        if constexpr (C::dst_min() < kSrcMin) {
            SM_ASSERTF(value <= kSrcMax, "value {} would overflow (limit: {})", value, kSrcMax);
        }
#endif

        return static_cast<T>(value);
    }

    template<typename T, typename O> requires IsEnum<T> || IsEnum<O>
    T enum_cast(O value) noexcept {
        if constexpr (IsEnum<T>) {
            using U = __underlying_type(T);
            return T(int_cast<U>(value));
        } else {
            return T(value);
        }
    }

    template<typename T>
    constexpr T roundup_pow2(T value) noexcept {
        T result = 1;
        while (result < value) {
            result <<= 1;
        }

        return result;
    }

    template<typename T>
    constexpr T roundup(T value, T multiple) noexcept {
        if (value == 0)
            return multiple;

        return (value + multiple - 1) / multiple * multiple;
    }
}
