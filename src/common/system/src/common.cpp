#include "common.hpp"

#include "core/arena.hpp"

#include "core/win32.h" // IWYU pragma: export

#include "os/os.h"

NORETURN
assert_last_error(panic_t panic, const char *expr) {
    sm::IArena *arena = sm::get_debug_arena();
    DWORD last_error = GetLastError();
    char *message = os_error_string(last_error, arena);

    ctpanic(panic, "win32 error: %s %s", message, expr);
}
