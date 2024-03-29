#pragma once

#include "logs/logs.hpp"
#include "core/win32.hpp" // IWYU pragma: export

extern HINSTANCE gInstance;
extern LPTSTR gWindowClass;
extern size_t gTimerFrequency;

LOG_CATEGORY(gSystemLog);

size_t query_timer();

// like assert but non-fatal if the api call fails
#define SM_CHECK_WIN32(expr)                                                                 \
    [&](source_info_t where) -> bool {                                                             \
        if (auto result = (expr); !result) {                                                       \
            gSystemLog.error("[{}:{}] {}: " #expr " = {}. {}", where.file, where.line, where.function, \
                         result, sm::sys::get_last_error());                                       \
            return false;                                                                          \
        }                                                                                          \
        return true;                                                                               \
    }(CT_SOURCE_CURRENT)
// pass CT_SOURCE_CURRENT as an arg to preserve source location
