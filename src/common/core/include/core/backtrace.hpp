#pragma once

#include "backtrace/backtrace.h"

#include "os/core.h"

#include "core/source_info.h"
#include "fmtlib/format.h" // IWYU pragma: keep

#include <string_view>

namespace sm {
    class ISystemError : public bt_error_t {
        static void wrap_begin(size_t error, void *user);
        static void wrap_frame(const bt_frame_t *frame, void *user);
        static void wrap_end(void *user);

    protected:
        virtual void error_begin(os_error_t error) = 0;
        virtual void error_frame(const bt_frame_t *frame) = 0;
        virtual void error_end() = 0;

        constexpr ISystemError() {
            begin = wrap_begin;
            next = wrap_frame;
            end = wrap_end;
            user = this;
        }
    };

    CT_NORETURN
    panic(source_info_t info, std::string_view msg);
}

#define SM_ASSERTF(expr, ...) \
    do { \
        if (auto check_result = (expr); !check_result) { \
            sm::panic(CT_SOURCE_CURRENT, fmt::format(__VA_ARGS__)); \
        } \
    } while (0)

#define SM_ASSERT(expr) SM_ASSERTF(expr, "assertion failed: {}", #expr)
#define SM_NEVER(...) sm::panic(CT_SOURCE_CURRENT, fmt::format(__VA_ARGS__))
