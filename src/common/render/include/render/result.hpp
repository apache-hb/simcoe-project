#pragma once

#include "core/win32.hpp" // IWYU pragma: export

#include "fmtlib/format.h" // IWYU pragma: export

namespace sm::render {
    class Result {
        HRESULT mValue;

    public:
        constexpr Result(HRESULT value) : mValue(value) {}

        constexpr operator HRESULT() const { return mValue; }

        constexpr operator bool() const { return SUCCEEDED(mValue); }
        constexpr bool success() const { return SUCCEEDED(mValue); }

        char *to_string() const;
    };
}

template<>
struct fmt::formatter<sm::render::Result> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    auto format(const sm::render::Result& result, fmt::format_context& ctx) const {
        return format_to(ctx.out(), "{}", result.to_string());
    }
};
