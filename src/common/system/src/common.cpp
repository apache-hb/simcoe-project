#include "common.hpp"

#include "system/system.hpp"

#include <errhandlingapi.h>

using namespace sm;

HINSTANCE gInstance = nullptr;
LPTSTR gWindowClass = nullptr;

CT_NORETURN
system::assertLastError(source_info_t panic, const char *expr) {
    sm::vpanic(panic, "win32 {}: {}", getLastError(), expr);
}
