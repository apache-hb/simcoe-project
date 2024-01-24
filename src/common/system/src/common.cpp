#include "common.hpp"

#include "core/arena.hpp"

#include "core/win32.h" // IWYU pragma: export

#include "os/os.h"

char *get_last_error(void) {
    DWORD last_error = GetLastError();
    sm::IArena *arena = sm::get_debug_arena();
    return os_error_string(last_error, arena);
}

NORETURN
assert_last_error(source_info_t panic, const char *expr) {
    char *message = get_last_error();
    ctu_panic(panic, "win32 error: %s %s", message, expr);
}
