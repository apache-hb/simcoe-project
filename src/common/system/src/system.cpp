#include "system/system.hpp"
#include "system/window.hpp"

#include "common.hpp"

#include "base/panic.h"

#include "resource.h"

#include <winbase.h>

using namespace sm;
using namespace sm::sys;

static constexpr const char *kClassName = "simcoe";

#if 0
static constexpr size_t kPathMax = 512;
static TCHAR gExecutablePath[kPathMax];
static DWORD gExecutablePathLength = 0;

const char *sys::get_exe_path() {
    CTASSERTF(gExecutablePathLength != 0, "system::get_exe_path() called before system::create()");

    return gExecutablePath;
}
#endif

void sys::create(HINSTANCE hInstance, logs::ILogger &logger) {
    CTASSERTF(hInstance != nullptr, "system::create() invalid hInstance");
    CTASSERTF(gWindowClass == nullptr, "system::create() called twice");

    SystemSink sink{logger};

    gInstance = hInstance;

    HICON hIcon = LoadIconA(
        /* hInst = */ hInstance,
        /* name = */ MAKEINTRESOURCEA(IDI_DEFAULT_ICON));

    if (hIcon == nullptr) {
        sink.warn("failed to load icon {}", last_error_string());
    }

    HCURSOR hCursor = LoadCursorA(
        /* hInst = */ nullptr,
        /* name = */ IDC_ARROW);

    if (hCursor == nullptr) {
        sink.warn("failed to load cursor {}", last_error_string());
    }

    const WNDCLASSEXA kClass = {
        .cbSize = sizeof(WNDCLASSEX),

        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = Window::proc,
        .hInstance = hInstance,
        .hIcon = hIcon,
        .hCursor = hCursor,
        .lpszClassName = kClassName,
    };

    if (ATOM atom = RegisterClassExA(&kClass); atom == 0) {
        assert_last_error(CT_SOURCE_HERE, "RegisterClassExA");
    } else {
        gWindowClass = MAKEINTATOM(atom);
    }

#if 0
    gExecutablePathLength = GetModuleFileNameA(
        /* hModule = */ nullptr,
        /* lpFilename = */ gExecutablePath,
        /* nSize = */ kPathMax);

    if (gExecutablePathLength == 0) {
        assert_last_error(CT_SOURCE_HERE, "GetModuleFileNameA");
    }

    if (gExecutablePathLength == kPathMax) {
        sink.warn("executable path longer than {}, may be truncated", kPathMax);
    }
#endif
}

void sys::destroy(void) {
    CTASSERTF(gInstance != nullptr, "system::destroy() called before system::create()");
    CTASSERTF(gWindowClass != nullptr, "system::destroy() called before system::create()");

    SM_ASSERT_WIN32(UnregisterClassA(gWindowClass, gInstance));

    gInstance = nullptr;
    gWindowClass = nullptr;
}
