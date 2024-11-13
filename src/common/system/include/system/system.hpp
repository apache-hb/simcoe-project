#pragma once

#include <simcoe_config.h>

#include "core/source_info.h"

#include "core/error.hpp"
#include "core/fs.hpp"
#include "logs/logs.hpp" // IWYU pragma: export

#include "logs/logging.hpp"

#include "core/win32.hpp"

LOG_MESSAGE_CATEGORY(SystemLog, "System");

namespace sm::system {
    enum class ShowWindow {
        eHide = SW_HIDE,
        eShowNormal = SW_SHOWNORMAL,
        eShow = SW_SHOW,
        eRestore = SW_RESTORE,
        eShowDefault = SW_SHOWDEFAULT,
    };

    enum class MultiMonitor {
        eNull = MONITOR_DEFAULTTONULL,
        ePrimary = MONITOR_DEFAULTTOPRIMARY,
        eNearest = MONITOR_DEFAULTTONEAREST,
    };

    enum class WindowMode : DWORD {
        eWindowed = WS_OVERLAPPEDWINDOW,
        eBorderless = WS_POPUP,
    };

    CT_NORETURN assertLastError(source_info_t panic, const char *expr);

    OsError getLastError();

    fs::path getProgramFolder();
    fs::path getProgramPath();
    std::string getProgramName();

    void create(HINSTANCE hInstance);
    void destroy(void) noexcept;
}

#define SM_ASSERT_WIN32(expr)                                  \
    do {                                                       \
        if (auto result = (expr); !result) {                   \
            sm::system::assertLastError(CT_SOURCE_CURRENT, #expr); \
        }                                                      \
    } while (0)
