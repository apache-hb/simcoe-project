#include "system/system.hpp"
#include "system/window.hpp"

#include "common.hpp"

#include "resource.h"

using namespace sm;
using namespace sm::sys;

static constexpr const char *kClassName = "simcoe";

static fs::path gProgramPath;
static fs::path gProgramDir;

fs::path sys::get_app_path() {
    return gProgramPath;
}

fs::path sys::get_appdir() {
    return gProgramDir;
}

fs::path sys::get_redist(const fs::path& name) {
    return gProgramDir / "client.exe.local" / name;
}

void sys::create(HINSTANCE hInstance) {
    CTASSERTF(hInstance != nullptr, "system::create() invalid hInstance");
    CTASSERTF(gWindowClass == nullptr, "system::create() called twice");

    auto sink = logs::get_sink(logs::Category::eSystem);

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
        .lpszClassName = kClassName,
    };

    if (ATOM atom = RegisterClassExA(&kClass); atom == 0) {
        assert_last_error(CT_SOURCE_CURRENT, "RegisterClassExA");
    } else {
        gWindowClass = MAKEINTATOM(atom);
    }

    LARGE_INTEGER frequency;
    SM_ASSERT_WIN32(QueryPerformanceFrequency(&frequency));

    gTimerFrequency = frequency.QuadPart;

    static constexpr size_t kPathMax = 2048;
    TCHAR gExecutablePath[kPathMax];
    DWORD gExecutablePathLength = 0;

    gExecutablePathLength = GetModuleFileNameA(
        /* hModule = */ nullptr,
        /* lpFilename = */ gExecutablePath,
        /* nSize = */ kPathMax);

    if (gExecutablePathLength == 0) {
        assert_last_error(CT_SOURCE_CURRENT, "GetModuleFileNameA");
    }

    if (gExecutablePathLength == kPathMax) {
        sink.warn("executable path longer than {}, may be truncated", kPathMax);
    }

    gProgramPath = fs::path{gExecutablePath, gExecutablePath + gExecutablePathLength};
    gProgramDir = gProgramPath.parent_path();
}

void sys::destroy(void) {
    CTASSERTF(gInstance != nullptr, "system::destroy() called before system::create()");
    CTASSERTF(gWindowClass != nullptr, "system::destroy() called before system::create()");

    SM_ASSERT_WIN32(UnregisterClassA(gWindowClass, gInstance));

    gInstance = nullptr;
    gWindowClass = nullptr;
}
