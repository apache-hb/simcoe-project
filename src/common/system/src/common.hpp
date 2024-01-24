#pragma once

#include "base/panic.h"
#include "core/compiler.h"

char *get_last_error(void);

NORETURN
assert_last_error(source_info_t panic, const char *expr);

#define SM_ASSERT_WIN32(expr)                         \
    do {                                              \
        if (auto result = (expr); !result) {          \
            assert_last_error(CT_SOURCE_HERE, #expr); \
        }                                             \
    } while (0)

// like assert but non-fatal if the api call fails
#define SM_CHECK_WIN32(expr, sink)                             \
    [&]() -> bool {                                            \
        if (auto result = (expr); !result) {                   \
            (sink).error(#expr " = {}. {}", get_last_error()); \
            return false;                                      \
        }                                                      \
        return true;                                           \
    }()
