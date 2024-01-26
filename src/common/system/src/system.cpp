#include "system/system.hpp"
#include "system/window.hpp"
#include "common.hpp"

#include "base/panic.h"
#include "resource.h"

#include <winbase.h>

using namespace sm;
using namespace sm::sys;

#define SM_CLASS_NAME "simcoe"

void sys::create(HINSTANCE hInstance, logs::ILogger &logger) {
    CTASSERTF(hInstance != nullptr, "system::create() invalid hInstance");
    CTASSERTF(gWindowClass == nullptr, "system::create() called twice");

    SystemSink sink{logger};

    gInstance = hInstance;

    HICON hIcon = LoadIconA(
        /* hInst = */ hInstance,
        /* name = */ MAKEINTRESOURCEA(IDI_DEFAULT_ICON));

    if (hIcon == nullptr) {
        sink.warn("failed to load icon {}", get_last_error());
    }

    HCURSOR hCursor = LoadCursorA(
        /* hInst = */ nullptr,
        /* name = */ IDC_ARROW);

    if (hCursor == nullptr) {
        sink.warn("failed to load cursor {}", get_last_error());
    }

    const WNDCLASSEXA kClass = {
        .cbSize = sizeof(WNDCLASSEX),

        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = Window::proc,
        .hInstance = hInstance,
        .hIcon = hIcon,
        .hCursor = hCursor,
        .lpszClassName = SM_CLASS_NAME,
    };

    if (ATOM atom = RegisterClassExA(&kClass); atom == 0) {
        assert_last_error(CT_SOURCE_HERE, "RegisterClassExA");
    } else {
        gWindowClass = MAKEINTATOM(atom);
    }
}

void sys::destroy(void) {
    CTASSERTF(gInstance != nullptr, "system::destroy() called before system::create()");
    CTASSERTF(gWindowClass != nullptr, "system::destroy() called before system::create()");

    SM_ASSERT_WIN32(UnregisterClassA(gWindowClass, gInstance));

    gInstance = nullptr;
    gWindowClass = nullptr;
}
