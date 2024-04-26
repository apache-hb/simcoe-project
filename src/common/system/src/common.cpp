#include "common.hpp"

#include "system/system.hpp"

#include <errhandlingapi.h>

using namespace sm;

LOG_CATEGORY_IMPL(gSystemLog, "system");

HINSTANCE gInstance = nullptr;
LPTSTR gWindowClass = nullptr;

OsError sys::getLastError() {
    return GetLastError();
}

CT_NORETURN
sys::assert_last_error(source_info_t panic, const char *expr) {
    sm::vpanic(panic, "win32 {}: {}", getLastError(), expr);
}
