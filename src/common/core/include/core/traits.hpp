#pragma once

#include <atomic>
#include <type_traits>

namespace sm {
    template<typename T>
    inline constexpr bool kIsIntegral = std::is_integral_v<T>;

    template<typename T>
    inline constexpr bool kIsIntegral<std::atomic<T>> = std::is_integral_v<T>;

    template<typename T>
    concept Integral = kIsIntegral<T>;
}
