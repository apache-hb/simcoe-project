#include "system/system.hpp"

#include "core/arena.hpp"

#include "common.hpp"

#include "os/core.h"

#include <errhandlingapi.h>

using namespace sm;

HINSTANCE gInstance = nullptr;
LPTSTR gWindowClass = nullptr;

char *sys::last_error_string(void) {
    DWORD last_error = GetLastError();
    IArena& arena = sm::get_pool(sm::Pool::eDebug);
    return os_error_string(last_error, &arena);
}

CT_NORETURN
sys::assert_last_error(source_info_t panic, const char *expr) {
    char *message = sys::last_error_string();
    ctu_panic(panic, "win32 error: %s %s", message, expr);
}
