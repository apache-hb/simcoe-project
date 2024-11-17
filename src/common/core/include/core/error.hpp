#pragma once

#include <simcoe_config.h>

#include "core/adt/small_string.hpp"
#include "core/error/error.hpp"

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

    class OsError : public errors::Error<OsError> {
        using Super = errors::Error<OsError>;
        os_error_t mError;

        OsError(os_error_t error, std::string_view message);

    public:
        using Exception = OsException;

        OsError(os_error_t error);

        template<typename... A>
        OsError(os_error_t error, fmt::format_string<A...> fmt, A&&... args)
            : OsError(error, fmt::vformat(fmt, fmt::make_format_args(args...)))
        { }

        constexpr os_error_t error() const noexcept { return mError; }
        constexpr bool isSuccess() const noexcept { return mError == 0; }
    };

    class OsException : public errors::Exception<OsError> {
        using Super = errors::Exception<OsError>;

    public:
        using Super::Super;

        template<typename... A>
        OsException(os_error_t error, fmt::format_string<A...> fmt, A&&... args)
            : Super(OsError(error, fmt, args...))
        { }
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
