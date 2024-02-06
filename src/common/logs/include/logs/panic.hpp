#pragma once

#include "logs/logs.hpp" // IWYU pragma: export
#include "base/panic.h"

namespace sm {
CT_NORETURN panic(source_info_t info, std::string_view msg);

CT_NORETURN vpanic(source_info_t info, std::string_view msg, auto&&... args) {
    panic(info, fmt::vformat(msg, fmt::make_format_args(args...)));
}
} // namespace sm

#define SM_ASSERTF_ALWAYS(expr, ...) \
    do {                                  \
        if (!(expr)) {                    \
            sm::vpanic(CT_SOURCE_HERE, __VA_ARGS__); \
        }                                 \
    } while (0)

#if SMC_DEBUG
#   define SM_ASSERTF(expr, msg, ...) SM_ASSERTF_ALWAYS(expr, msg, __VA_ARGS__)
#else
#   define SM_ASSERTF(expr) CT_ASSUME(expr)
#endif

#define SM_ASSERTM(expr, msg) SM_ASSERTF(expr, "{}", msg)

#define SM_ASSERT(expr) SM_ASSERTM(expr, #expr)

#define SM_NEVER(...) SM_ASSERTF_ALWAYS(false, __VA_ARGS__)
