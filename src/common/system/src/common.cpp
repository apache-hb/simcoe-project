#include "common.hpp"

#include "system/system.hpp"

#include <errhandlingapi.h>

using namespace sm;

HINSTANCE gInstance = nullptr;
LPTSTR gWindowClass = nullptr;
size_t gTimerFrequency = 0;

OsError sys::get_last_error() {
    return GetLastError();
}

CT_NORETURN
sys::assert_last_error(source_info_t panic, const char *expr) {
    sm::vpanic(panic, "win32 {}: {}", get_last_error(), expr);
}

size_t query_timer() {
    CTASSERT(gTimerFrequency != 0);

    LARGE_INTEGER frequency;
    SM_ASSERT_WIN32(QueryPerformanceCounter(&frequency));

    return int_cast<size_t>(frequency.QuadPart);
}
