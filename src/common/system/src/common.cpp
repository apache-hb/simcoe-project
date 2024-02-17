#include "common.hpp"

#include "system/system.hpp"

#include <errhandlingapi.h>

using namespace sm;

HINSTANCE gInstance = nullptr;
LPTSTR gWindowClass = nullptr;

OsError sys::get_last_error() {
    return GetLastError();
}

CT_NORETURN
sys::assert_last_error(source_info_t panic, const char *expr) {
    sm::vpanic(panic, "win32 {}: {}", get_last_error(), expr);
}
