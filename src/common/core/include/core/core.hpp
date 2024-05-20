#pragma once

#include <simcoe_config.h>

#include <stdint.h>

#include <concepts>

namespace sm {
    using uint8 = uint8_t; // NOLINT
    using uint16 = uint16_t; // NOLINT
    using uint32 = uint32_t; // NOLINT
    using uint64 = uint64_t; // NOLINT

    using int8 = int8_t; // NOLINT
    using int16 = int16_t; // NOLINT
    using int32 = int32_t; // NOLINT
    using int64 = int64_t; // NOLINT

    using uint = uint32_t; // NOLINT
    using byte = uint8_t; // NOLINT

    using char8 = char8_t; // NOLINT
    using char16 = char16_t; // NOLINT
    using char32 = char32_t; // NOLINT

    using ssize_t = ptrdiff_t; // NOLINT

    template<typename T>
    struct Empty {
        constexpr Empty() = default;
        constexpr Empty(const T&) { }
        constexpr Empty(T&&) { }

        constexpr void operator=(T&&) const { }
    };

    constexpr bool isPowerOf2(auto it) {
        return it && !(it & (it - 1));
    }

    template<typename T>
    constexpr bool isMultipleOf(T value, T multiple) {
        return value % multiple == 0;
    }

    template<typename T>
    concept numeric = std::integral<T> || std::floating_point<T>;

    bool isInStaticStorage(const void *ptr) noexcept;

    bool isStaticStorage(const auto *ptr) noexcept {
        return isInStaticStorage((const void *)ptr);
    }
}

/// @brief debug member variables and required macros
/// SM_DBG_MEMBER(type) declares a member variable that will only be present in debug builds
/// SM_DBG_REF(name) allows storing to a variable that will only be present in debug builds
/// all debug variables can only be read inside a SM_DBG_ASSERT block or inside a
/// #if SMC_DEBUG block

#if SMC_DEBUG
#   define DBG_MEMBER(T) T
#   define DBG_MEMBER_OR(name, ...) name
#   define DBG_ASSERT(expr, ...) CTASSERTF(expr, __VA_ARGS__)
#else
#   define DBG_MEMBER(T) SM_NO_UNIQUE_ADDRESS sm::Empty<T>
#   define DBG_MEMBER_OR(name, ...) __VA_ARGS__
#   define DBG_ASSERT(expr, ...) do { } while (0)
#endif

#if SMC_RELEASE
#   define DBG_STATIC_ASSERT(...) static_assert(__VA_ARGS__)
#else
#   define DBG_STATIC_ASSERT(...) static_assert(true)
#endif

#if defined(__clang__)
#   define undefined(T) ([] { T ud = __builtin_nondeterministic_value(ud); return ud; }())
#else
#   define undefined(T) (T{})
#endif
