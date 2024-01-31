#pragma once

#include <simcoe_config.h>

#include "base/panic.h"

#include "system.reflect.h" // IWYU pragma: export

typedef struct arena_t arena_t;

namespace sm {
class IArena;
}

#define SM_ASSERT_WIN32(expr)                                  \
    do {                                                       \
        if (auto result = (expr); !result) {                   \
            sm::sys::assert_last_error(CT_SOURCE_HERE, #expr); \
        }                                                      \
    } while (0)

// like assert but non-fatal if the api call fails
#define SM_CHECK_WIN32(expr, sink)                                                                 \
    [&](source_info_t where) -> bool {                                                             \
        if (auto result = (expr); !result) {                                                       \
            (sink).error("[{}:{}] {}: " #expr " = {}. {}", where.file, where.line, where.function, \
                         result, sm::sys::last_error_string());                                       \
            return false;                                                                          \
        }                                                                                          \
        return true;                                                                               \
    }(CT_SOURCE_HERE)
// pass CT_SOURCE_HERE as an arg to preserve source location

namespace sm::sys {
using SystemSink = logs::Sink<logs::Category::eSystem>;

CT_NORETURN
assert_last_error(source_info_t panic, const char *expr);

char *last_error_string();

const char *get_exe_path();

void create(HINSTANCE hInstance, logs::ILogger &logger);
void destroy(void);
} // namespace sm::sys
