#pragma once

#include <simcoe_config.h>

#include <stdint.h>

#include <concepts>

namespace sm {
    // NOLINTBEGIN
    using uint8 = uint8_t;
    using uint16 = uint16_t;
    using uint32 = uint32_t;
    using uint64 = uint64_t;

    using int8 = int8_t;
    using int16 = int16_t;
    using int32 = int32_t;
    using int64 = int64_t;

    using uint = uint32_t;
    using byte = uint8_t;

    using char8 = char8_t;
    using char16 = char16_t;
    using char32 = char32_t;

    using ssize_t = std::ptrdiff_t;

    struct noinit { };
    struct init { };
    // NOLINTEND

    // basic checks on elements in a container to make them
    // somewhat sane to work with. I require noexcept move and destruct
    // because not having these makes writing exception safe
    // containers nearly impossible.
    template<typename T>
    concept SafeObject = std::is_nothrow_move_assignable_v<T>
                      && std::is_nothrow_move_constructible_v<T>
                      && std::is_nothrow_destructible_v<T>;

    template<typename T>
    struct Empty {
        constexpr Empty() noexcept = default;
        constexpr Empty(const T&) noexcept(std::is_nothrow_copy_constructible_v<T>) { }
        constexpr Empty(T&&) noexcept(std::is_nothrow_move_constructible_v<T>) { }

        constexpr void operator=(T&&) const noexcept(std::is_nothrow_move_constructible_v<T>) { }
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

    bool isInStaticStorage(const void *ptr, bool fallback = false) noexcept;

    bool isStaticStorage(const auto *ptr, bool fallback = false) noexcept {
        return isInStaticStorage((const void *)ptr, fallback);
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
