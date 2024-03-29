#pragma once

#include <simcoe_config.h>

#include "core/compiler.h"
#include "core/source_info.h"

#include "core/error.hpp"
#include "core/fs.hpp"
#include "logs/logs.hpp" // IWYU pragma: export

#include "system.reflect.h" // IWYU pragma: export

namespace sm::sys {
    CT_NORETURN assert_last_error(source_info_t panic, const char *expr);

    OsError get_last_error();

    fs::path get_app_path();
    fs::path get_appdir();

    void create(HINSTANCE hInstance);
    void destroy(void);
}

#define SM_ASSERT_WIN32(expr)                                  \
    do {                                                       \
        if (auto result = (expr); !result) {                   \
            sm::sys::assert_last_error(CT_SOURCE_CURRENT, #expr); \
        }                                                      \
    } while (0)
