#pragma once

#include <simcoe_config.h>

#include <limits>

#include "core/core.hpp"

#include "core.reflect.h"

namespace sm {
    template<typename T, typename O>
    struct CastResult {
        using From = O;
        using To = T;

        static constexpr T dst_min() { return std::numeric_limits<T>::min(); }
        static constexpr T dst_max() { return std::numeric_limits<T>::max(); }

        static constexpr O src_min() { return std::numeric_limits<O>::min(); }
        static constexpr O src_max() { return std::numeric_limits<O>::max(); }

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
    T enum_cast(O value) {
        if constexpr (IsEnum<T>) {
            using U = __underlying_type(T);
            return T(int_cast<U>(value));
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
