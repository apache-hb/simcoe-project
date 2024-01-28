#pragma once

#include <atomic>
#include <type_traits>

#include "base/panic.h"

namespace sm {
    template<typename T>
    inline constexpr bool kIsIntegral = std::is_integral_v<T>;

    template<typename T>
    inline constexpr bool kIsIntegral<std::atomic<T>> = std::is_integral_v<T>;

    template<typename T>
    concept Integral = kIsIntegral<T>;
}

#define SM_ASSERTF(expr, fmt, ...)                 \
    do {                                      \
        if (std::is_constant_evaluated()) {   \
            static_assert(expr, fmt); \
        } else {                              \
            CTASSERTF(expr, fmt, __VA_ARGS__);     \
        }                                     \
    } while (0)
