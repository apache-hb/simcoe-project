#pragma once

#include <simcoe_config.h>

#include "core/adt/small_string.hpp"
#include "core/throws.hpp"

#include "backtrace/backtrace.h"
#include "core/source_info.h"

#include "fmtlib/format.h" // IWYU pragma: keep
#include "os/core.h"

#include <string_view>

namespace sm {
    class OsException;

    CT_NORETURN panic(source_info_t info, std::string_view msg);

    CT_NORETURN vpanic(source_info_t info, std::string_view msg, auto &&...args) {
        panic(info, fmt::vformat(msg, fmt::make_format_args(args...)));
    }

    class OsError {
        os_error_t mError;

    public:
        constexpr OsError(os_error_t error) noexcept
            : mError(error)
        { }

        constexpr os_error_t error() const noexcept { return mError; }
        constexpr operator bool() const noexcept { return mError != 0; }
        constexpr bool success() const noexcept { return mError == 0; }
        constexpr bool failed() const noexcept { return mError != 0; }

        constexpr bool operator==(const OsError &) const noexcept = default;
        constexpr bool operator!=(const OsError &) const noexcept = default;

        CT_NORETURN raise() const throws(OsException);
        void throwIfFailed() const throws(OsException);

        template<size_t N = 512>
        SmallString<N> toString() const {
            char buffer[N];
            size_t size = os_error_get_string(mError, buffer, N);
            buffer[std::min(size, N - 1)] = '\0';
            return SmallString<N>(buffer);
        }
    };

    class OsException : public std::exception {
        OsError mError;
        std::string mMessage;

    public:
        OsException(OsError error)
            : mError(error)
            , mMessage(error.toString())
        { }

        OsError error() const noexcept { return mError; }
        const char *what() const noexcept override { return mMessage.c_str(); }
    };

    class ISystemError : public bt_error_t {
        static void wrap_begin(size_t error, void *user);
        static void wrap_frame(bt_address_t, void *user);
        static void wrap_end(void *user);

    protected:
        virtual void error_begin(OsError error) = 0;
        virtual void error_frame(bt_address_t) = 0;
        virtual void error_end() = 0;

        constexpr ISystemError() noexcept
            : bt_error_t{wrap_begin, wrap_end, wrap_frame, this}
        { }
    };
} // namespace sm

template <>
struct fmt::formatter<sm::OsError> {
    constexpr auto parse(format_parse_context &ctx) const {
        return ctx.begin();
    }

    auto format(sm::OsError error, fmt::format_context &ctx) const {
        return format_to(ctx.out(), "{}", error.toString<512>().c_str());
    }
};

#define SM_ASSERTF_ALWAYS(expr, ...)                    \
    do {                                                \
        if (!(expr)) {                                  \
            sm::vpanic(CT_SOURCE_CURRENT, __VA_ARGS__); \
        }                                               \
    } while (0)

#if SMC_DEBUG
#   define SM_ASSERTF(expr, ...) SM_ASSERTF_ALWAYS(expr, __VA_ARGS__)
#else
#   define SM_ASSERTF(expr, ...) CT_ASSUME(expr)
#endif

#define SM_ASSERTM(expr, msg) SM_ASSERTF(expr, "{}", msg)
#define SM_ASSERT(expr) SM_ASSERTM(expr, #expr)

#if SMC_DEBUG
#   define SM_NEVER(...) SM_ASSERTF(false, __VA_ARGS__)
#else
#   define SM_NEVER(...) SM_ASSERTF_ALWAYS(false, __VA_ARGS__)
#endif
