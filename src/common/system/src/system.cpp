#include "system/system.hpp"
#include "system/window.hpp"

#include "common.hpp"

#include "resource.h"

using namespace sm;

static constexpr const char *kClassName = "simcoe";

static bool isSystemSetup() noexcept {
    return gWindowClass != nullptr && gInstance != nullptr;
}

void system::create(HINSTANCE hInstance) {
    CTASSERTF(hInstance != nullptr, "system::create() invalid hInstance");

    if (isSystemSetup()) {
        CT_NEVER("system::create() called twice");
    }

    gInstance = hInstance;

    HICON hIcon = LoadIconA(
        /* hInst = */ hInstance,
        /* name = */ MAKEINTRESOURCEA(IDI_DEFAULT_ICON)
    );

    if (hIcon == nullptr) {
        LOG_WARN(SystemLog, "failed to load icon {}", getLastError());
    }

    HCURSOR hCursor = LoadCursorA(nullptr, IDC_ARROW);

    if (hCursor == nullptr) {
        LOG_WARN(SystemLog, "failed to load cursor {}", getLastError());
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
        assertLastError(CT_SOURCE_CURRENT, "RegisterClassExA");
    } else {
        gWindowClass = MAKEINTATOM(atom);
    }

    initStorage();
}

void system::destroy(void) noexcept {
    if (!isSystemSetup())
        return;

    SM_ASSERT_WIN32(UnregisterClassA(gWindowClass, gInstance));

    gInstance = nullptr;
    gWindowClass = nullptr;
}
