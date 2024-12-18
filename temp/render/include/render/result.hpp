#pragma once

#include "core/adt/small_string.hpp"
#include "render/base/error.hpp"

#include "core/win32.hpp" // IWYU pragma: export

#include "fmtlib/format.h" // IWYU pragma: export

#include "logs/logging.hpp"

LOG_MESSAGE_CATEGORY(GpuLog, "D3D12");
LOG_MESSAGE_CATEGORY(RenderLog, "Render");

namespace sm::render {
    class Result {
        HRESULT mValue;

    public:
        constexpr Result(HRESULT value) : mValue(value) {}

        constexpr operator bool() const { return SUCCEEDED(mValue); }
        constexpr bool success() const { return SUCCEEDED(mValue); }
        constexpr bool failed() const { return FAILED(mValue); }

        constexpr explicit operator HRESULT() const { return mValue; }

        std::string_view toString() const {
            OsError err = OsError(mValue);
            return err.message();
        }
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
        return format_to(ctx.out(), "{}", result.toString());
    }
};

#define SM_ASSERT_HR(expr)                                 \
    do {                                                   \
        if (sm::render::Result check_result = (expr); !check_result) {        \
            sm::assert_hresult(CT_SOURCE_CURRENT, check_result, #expr); \
        }                                                  \
    } while (0)
