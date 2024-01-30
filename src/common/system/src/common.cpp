#include "system/system.hpp"

#include "core/arena.hpp"

#include "os/os.h"
#include "common.hpp"

#include <errhandlingapi.h>

HINSTANCE gInstance = nullptr;
LPTSTR gWindowClass = nullptr;

char *sm::sys::get_last_error(void) {
    DWORD last_error = GetLastError();
    sm::IArena& arena = sm::get_pool(sm::Pool::eDebug);
    return os_error_string(last_error, &arena);
}

CT_NORETURN
sm::sys::assert_last_error(source_info_t panic, const char *expr) {
    char *message = sm::sys::get_last_error();
    ctu_panic(panic, "win32 error: %s %s", message, expr);
}
