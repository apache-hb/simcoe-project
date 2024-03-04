#pragma once

#include <simcoe_config.h>

#include "core/compiler.h"
#include "core/source_info.h"

#include "core/error.hpp"
#include "core/text.hpp"
#include "logs/logs.hpp" // IWYU pragma: export

#include "system.reflect.h" // IWYU pragma: export

#define SM_ASSERT_WIN32(expr)                                  \
    do {                                                       \
        if (auto result = (expr); !result) {                   \
            sm::sys::assert_last_error(CT_SOURCE_CURRENT, #expr); \
        }                                                      \
    } while (0)

// like assert but non-fatal if the api call fails
#define SM_CHECK_WIN32(expr)                                                                 \
    [&](source_info_t where) -> bool {                                                             \
        if (auto result = (expr); !result) {                                                       \
            logs::get_sink(logs::Category::eSystem).error("[{}:{}] {}: " #expr " = {}. {}", where.file, where.line, where.function, \
                         result, sm::sys::get_last_error());                                       \
            return false;                                                                          \
        }                                                                                          \
        return true;                                                                               \
    }(CT_SOURCE_CURRENT)
// pass CT_SOURCE_CURRENT as an arg to preserve source location

namespace sm::sys {
CT_NORETURN
assert_last_error(source_info_t panic, const char *expr);

OsError get_last_error();

using OsString = sm::CoreString<TCHAR>;
using OsStringView = sm::CoreStringView<TCHAR>;

//const char *get_exe_path();

void create(HINSTANCE hInstance);
void destroy(void);
} // namespace sm::sys
