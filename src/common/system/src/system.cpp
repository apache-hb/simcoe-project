#include "system/system.hpp"
#include "system/window.hpp"

#include "common.hpp"

#include "resource.h"

using namespace sm;

namespace sys = sm::system;

static constexpr const char *kClassName = "simcoe";

static fs::path gProgramPath;
static fs::path gProgramDir;

fs::path sys::getProgramFolder() {
    return gProgramDir;
}

fs::path sys::getProgramPath() {
    return gProgramPath;
}

void sys::create(HINSTANCE hInstance) {
    CTASSERTF(hInstance != nullptr, "system::create() invalid hInstance");
    CTASSERTF(gWindowClass == nullptr, "system::create() called twice");

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

    static constexpr size_t kPathMax = 0x1000;
    TCHAR gExecutablePath[kPathMax];
    DWORD gExecutablePathLength = 0;

    gExecutablePathLength = GetModuleFileNameA(nullptr, gExecutablePath, kPathMax);

    if (gExecutablePathLength == 0) {
        assertLastError(CT_SOURCE_CURRENT, "GetModuleFileNameA");
    }

    if (gExecutablePathLength >= kPathMax) {
        LOG_WARN(SystemLog, "executable path longer than {}, may be truncated", kPathMax);
    }

    gProgramPath = fs::path{gExecutablePath, gExecutablePath + gExecutablePathLength};
    gProgramDir = gProgramPath.parent_path();
}

void sys::destroy(void) noexcept {
    CTASSERTF(gInstance != nullptr, "system::destroy() called before system::create()");
    CTASSERTF(gWindowClass != nullptr, "system::destroy() called before system::create()");

    SM_ASSERT_WIN32(UnregisterClassA(gWindowClass, gInstance));

    gInstance = nullptr;
    gWindowClass = nullptr;
}
