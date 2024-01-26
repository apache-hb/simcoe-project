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
                         result, sm::sys::get_last_error());                                       \
            return false;                                                                          \
        }                                                                                          \
        return true;                                                                               \
    }(CT_SOURCE_HERE)

namespace sm::sys {
using SystemSink = logs::Sink<logs::Category::eSystem>;

NORETURN
assert_last_error(source_info_t panic, const char *expr);

char *get_last_error();

void create(HINSTANCE hInstance, logs::ILogger &logger);
void destroy(void);
} // namespace sm::sys
