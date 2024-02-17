#pragma once

#include "core/win32.hpp" // IWYU pragma: export

#include "fmtlib/format.h" // IWYU pragma: export

namespace sm::render {
    class Result {
        HRESULT mValue;

    public:
        constexpr Result(HRESULT value) : mValue(value) {}

        constexpr operator bool() const { return SUCCEEDED(mValue); }
        constexpr bool success() const { return SUCCEEDED(mValue); }

        constexpr explicit operator HRESULT() const { return mValue; }

        char *to_string() const;
    };
}

namespace sm {
    CT_NORETURN
    assert_hresult(source_info_t source, render::Result hr, std::string_view msg);
}

template<>
struct fmt::formatter<sm::render::Result> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const sm::render::Result& result, fmt::format_context& ctx) const {
        return format_to(ctx.out(), "{}", result.to_string());
    }
};

#define SM_ASSERT_HR(expr)                                 \
    do {                                                   \
        if (sm::render::Result check_result = (expr); !check_result) {        \
            sm::assert_hresult(CT_SOURCE_CURRENT, check_result, #expr); \
        }                                                  \
    } while (0)
