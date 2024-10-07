#include "system/window.hpp"
#include "core/format.hpp" // IWYU pragma: keep

#include "common.hpp"

#include "resource.h"

#include <winbase.h>

using namespace sm;

namespace sys = sm::system;
using Window = sys::Window;

// like assert but non-fatal if the api call fails
#define SM_CHECK_WIN32(expr)                                                                       \
    [&]() -> bool {                                                                                \
        if (auto result = (expr); !result) {                                                       \
            LOG_ERROR(SystemLog, #expr " = {}. {}", result, sm::system::getLastError());              \
            return false;                                                                          \
        }                                                                                          \
        return true;                                                                               \
    }()

LRESULT CALLBACK sys::Window::proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    Window *self = reinterpret_cast<Window*>(GetWindowLongPtrA(window, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE: {
        CREATESTRUCT *create = reinterpret_cast<CREATESTRUCT *>(lparam);
        Window *it = reinterpret_cast<Window *>(create->lpCreateParams);

        // CreateWindow calls WM_CREATE before returning
        // so we need to set the window handle here
        it->mWindow = window;

        SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(it));

        it->mEvents.create(*it);
        break;
    }
    case WM_SIZE: {
        if (self == nullptr) break;

        int width = LOWORD(lparam);
        int height = HIWORD(lparam);

        self->mEvents.resize(*self, {width, height});
        break;
    }
    case WM_CLOSE: {
        if (self == nullptr) break;

        if (self->mEvents.close(*self))
            self->destroy_window();

        break;
    }
    case WM_DESTROY:
        if (self != nullptr)
            self->mEvents.destroy(*self);

        PostQuitMessage(0);
        break;

    default:
        if (self == nullptr) break;
        if (LRESULT res = self->mEvents.event(*self, message, wparam, lparam))
            return res;

        break;
    }

    return DefWindowProcA(window, message, wparam, lparam);
}

void Window::create(const WindowConfig &info) {
    CTASSERTF(gInstance != nullptr, "system::create() not called before Window::create()");
    CTASSERTF(gWindowClass != nullptr, "system::create() not called before Window::create()");

    SM_ASSERTF(info.mode.is_valid(), "invalid mode: {}", info.mode);

    mWindow = CreateWindowExA(
        /* dwExStyle = */ 0,
        /* lpClassName = */ gWindowClass,
        /* lpWindowName = */ info.title,
        /* dwStyle = */ info.mode.as_integral(),
        /* x = */ CW_USEDEFAULT,
        /* y = */ CW_USEDEFAULT,
        /* nWidth = */ info.width,
        /* nHeight = */ info.height,
        /* hWndParent = */ nullptr,
        /* hMenu = */ nullptr,
        /* hInstance = */ gInstance,
        /* lpParam = */ this);

    SM_ASSERT_WIN32(mWindow != nullptr);
}

Window::Window(const WindowConfig &info, IWindowEvents& events)
    : mEvents(events)
{
    create(info);
}

Window::~Window() {
    if (mWindow != nullptr) destroy_window();
}

sys::WindowPlacement Window::getPlacement(void) const {
    WINDOWPLACEMENT placement{.length = sizeof(WINDOWPLACEMENT)};
    SM_ASSERT_WIN32(GetWindowPlacement(mWindow, &placement));

    return placement;
}

void Window::setPlacement(const WindowPlacement &placement) {
    SM_CHECK_WIN32(SetWindowPlacement(mWindow, &placement));
}

void Window::show_window(ShowWindow show) {
    CTASSERTF(mWindow != nullptr, "Window::show_window() called before Window::create()");
    SM_ASSERTF(show.is_valid(), "invalid show: {}", show);
    ::ShowWindow(mWindow, show.as_integral());
}

void Window::resize(math::int2 size) {
    CTASSERTF(mWindow != nullptr, "Window::resize() called before Window::create()");

    SM_CHECK_WIN32(SetWindowPos(mWindow, nullptr, 0, 0, size.x, size.y, SWP_NOMOVE | SWP_NOZORDER));
}

void Window::destroy_window() {
    SM_CHECK_WIN32(DestroyWindow(mWindow));
    mWindow = nullptr;
}

void Window::set_title(const char *title) {
    CTASSERT(title != nullptr);

    SM_CHECK_WIN32(SetWindowTextA(mWindow, title));
}

sys::WindowCoords Window::get_coords() const {
    RECT rect{};
    SM_ASSERT_WIN32(GetWindowRect(mWindow, &rect));

    return rect;
}

sys::WindowCoords Window::get_client_coords() const {
    RECT rect{};
    SM_ASSERT_WIN32(GetClientRect(mWindow, &rect));

    return rect;
}

bool Window::center_window(MultiMonitor monitor, bool topmost) {
    CTASSERTF(mWindow != nullptr, "Window::center_window() called before Window::create()");
    SM_ASSERTF(monitor.is_valid(), "invalid monitor {}", monitor);

    // get current monitor
    HMONITOR hmonitor = MonitorFromWindow(mWindow, monitor.as_integral());
    if (!SM_CHECK_WIN32(hmonitor != nullptr)) return false;

    // get monitor info
    MONITORINFO monitor_info{.cbSize = sizeof(monitor_info)};
    if (!SM_CHECK_WIN32(GetMonitorInfoA(hmonitor, &monitor_info))) return false;

    // get window rect
    RECT rect{};
    if (!SM_CHECK_WIN32(GetWindowRect(mWindow, &rect))) return false;

    // calculate center
    int x = (monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 -
            (rect.right - rect.left) / 2;
    int y = (monitor_info.rcWork.top + monitor_info.rcWork.bottom) / 2 -
            (rect.bottom - rect.top) / 2;

    HWND insert = topmost ? HWND_TOPMOST : HWND_TOP;
    UINT flags = topmost ? 0 : SWP_NOZORDER;

    // move window
    return SM_CHECK_WIN32(SetWindowPos(mWindow, insert, x, y, 0, 0, SWP_NOSIZE | flags));
}
