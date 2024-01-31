#pragma once

#include <simcoe_config.h>

#include "core/macros.hpp" // IWYU pragma: export

namespace sm {
    template<typename T>
    struct Empty {
        constexpr Empty() noexcept = default;
        constexpr Empty(const T&) noexcept { }
        constexpr Empty(T&&) noexcept { }
    };

    struct Ignore {
        template<typename T>
        constexpr void operator=(T&&) const noexcept { }
    };

    inline constexpr Ignore kIgnore{};
}

/// @brief debug member variables and required macros
/// SM_DBG_MEMBER(type) declares a member variable that will only be present in debug builds
/// SM_DBG_REF(name) allows storing to a variable that will only be present in debug builds
/// all debug variables can only be read inside a SM_DBG_ASSERT block or inside a
/// #if SMC_DEBUG block

#if SMC_DEBUG
#   define SM_DBG_MEMBER(T) T
#   define SM_DBG_REF(name) name
#   define SM_DBG_MEMBER_OR(name, ...) name
#   define SM_DBG_ASSERT(expr, ...) CTASSERTF(expr, __VA_ARGS__)
#else
#   define SM_DBG_MEMBER(T) SM_NO_UNIQUE_ADDRESS sm::Empty<T>
#   define SM_DBG_REF(name) sm::kIgnore
#   define SM_DBG_MEMBER_OR(name, ...) __VA_ARGS__
#   define SM_DBG_ASSERT(expr, ...) do { } while (0)
#endif

#if SMC_RELEASE
#   define SM_REL_STATIC_ASSERT(...) static_assert(__VA_ARGS__)
#else
#   define SM_REL_STATIC_ASSERT(...) static_assert(true, "")
#endif
