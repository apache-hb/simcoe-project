#pragma once

#include "backtrace/backtrace.h"
#include "core/source_info.h"

#include "fmtlib/format.h" // IWYU pragma: keep
#include "os/core.h"

#include <string_view>

namespace sm {
CT_NORETURN
panic(source_info_t info, std::string_view msg);

CT_NORETURN
vpanic(source_info_t info, std::string_view msg, auto &&...args) {
    panic(info, fmt::vformat(msg, fmt::make_format_args(args...)));
}

class OsError {
    os_error_t mError;

public:
    constexpr OsError(os_error_t error)
        : mError(error) {}

    constexpr os_error_t error() const {
        return mError;
    }
    constexpr operator bool() const {
        return mError != 0;
    }
    constexpr bool success() const {
        return mError == 0;
    }
    constexpr bool failed() const {
        return mError != 0;
    }

    constexpr bool operator==(const OsError &other) const {
        return mError == other.mError;
    }

    char *to_string() const;
};

class ISystemError : public bt_error_t {
    static void wrap_begin(size_t error, void *user);
    static void wrap_frame(const bt_frame_t *frame, void *user);
    static void wrap_end(void *user);

protected:
    virtual void error_begin(OsError error) = 0;
    virtual void error_frame(const bt_frame_t *frame) = 0;
    virtual void error_end() = 0;

    constexpr ISystemError() {
        begin = wrap_begin;
        next = wrap_frame;
        end = wrap_end;
        user = this;
    }
};
} // namespace sm

template <>
struct fmt::formatter<sm::OsError> {
    constexpr auto parse(format_parse_context &ctx) const {
        return ctx.begin();
    }

    auto format(const sm::OsError &error, fmt::format_context &ctx) const {
        return format_to(ctx.out(), "{}", error.to_string());
    }
};

#define SM_ASSERTF_ALWAYS(expr, ...)                    \
    do {                                                \
        if (!(expr)) {                                  \
            sm::vpanic(CT_SOURCE_CURRENT, __VA_ARGS__); \
        }                                               \
    } while (0)

#if SMC_DEBUG
#   define SM_ASSERTF(expr, msg, ...) SM_ASSERTF_ALWAYS(expr, msg, __VA_ARGS__)
#else
#   define SM_ASSERTF(expr) CT_ASSUME(expr)
#endif

#define SM_ASSERTM(expr, msg) SM_ASSERTF(expr, "{}", msg)

#define SM_ASSERT(expr) SM_ASSERTM(expr, #expr)

#define SM_NEVER(...) SM_ASSERTF_ALWAYS(false, __VA_ARGS__)
