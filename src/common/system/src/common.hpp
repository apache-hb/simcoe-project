#pragma once

#include "base/panic.h"
#include "core/compiler.h"

NORETURN
assert_last_error(panic_t panic, const char *expr);

#define SM_ASSERT_WIN32(expr) \
    if (auto result = (expr); !result) { \
        assert_last_error(CTU_PANIC_INFO, #expr); \
    }
