#include "system/window.hpp"
#include "common.hpp"

#include "base/panic.h"
#include "resource.h"

#include <winbase.h>

using namespace sm;
using namespace sm::sys;

LRESULT CALLBACK Window::proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    Window *self = reinterpret_cast<Window *>(GetWindowLongPtrA(window, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE: {
        CREATESTRUCT *create = reinterpret_cast<CREATESTRUCT *>(lparam);
        Window *it = reinterpret_cast<Window *>(create->lpCreateParams);

        // CreateWindow calls WM_CREATE before returning
        // so we need to set the window handle here
        it->m_window = window;

        SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(it));

        it->m_events->create(*it);
        break;
    }
    case WM_SIZE: {
        if (self == nullptr) break;

        int width = LOWORD(lparam);
        int height = HIWORD(lparam);

        self->m_events->resize(*self, {width, height});
        break;
    }

    case WM_CLOSE:
        // we may not have a window so use DestroyWindow directly
        if (self != nullptr) {
            if (self->m_events->close(*self)) self->destroy_window();
        } else {
            // dont check the result of DestroyWindow because we have no
            // log sink to report the error to
            DestroyWindow(window);
        }

        break;

    case WM_DESTROY:
        if (self != nullptr) self->m_events->destroy(*self);

        PostQuitMessage(0);
        break;

    default:
        if (self != nullptr && self->m_events->event(*self, message, wparam, lparam) != 0) return 0;

        return DefWindowProcA(window, message, wparam, lparam);
    }

    return 0;
}

void Window::create(const WindowConfig &info) {
    SM_UNUSED constexpr auto refl = ctu::reflect<WindowMode>();
    CTASSERTF(gInstance != nullptr, "system::create() not called before Window::create()");
    CTASSERTF(gWindowClass != nullptr, "system::create() not called before Window::create()");

    CTASSERTF(info.mode.is_valid(), "Window::create() invalid mode: %s",
              refl.to_string(info.mode, 16).data());

    m_window = CreateWindowExA(
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

    SM_ASSERT_WIN32(m_window != nullptr);
}

Window::Window(const WindowConfig &info, IWindowEvents *events)
    : m_events(events)
    , m_log(info.logger) {
    CTASSERT(events != nullptr);

    create(info);
}

Window::~Window() {
    if (m_window != nullptr) destroy_window();
}

WindowPlacement Window::get_placement(void) const {
    WINDOWPLACEMENT placement{.length = sizeof(WINDOWPLACEMENT)};
    SM_ASSERT_WIN32(GetWindowPlacement(m_window, &placement));

    return placement;
}

void Window::set_placement(const WindowPlacement &placement) {
    SM_CHECK_WIN32(SetWindowPlacement(m_window, &placement), m_log);
}

void Window::show_window(ShowWindow show) {
    CTASSERTF(m_window != nullptr, "Window::show_window() called before Window::create()");
    CTASSERTF(show.is_valid(), "Window::show_window() invalid show: %d", show.as_integral());
    ::ShowWindow(m_window, show.as_integral());
}

void Window::destroy_window() {
    SM_CHECK_WIN32(DestroyWindow(m_window), m_log);
    m_window = nullptr;
}

void Window::set_title(const char *title) {
    CTASSERT(title != nullptr);

    SM_CHECK_WIN32(SetWindowTextA(m_window, title), m_log);
}

WindowCoords Window::get_coords() const {
    RECT rect{};
    SM_ASSERT_WIN32(GetWindowRect(m_window, &rect));

    return rect;
}

WindowCoords Window::get_client_coords() const {
    RECT rect{};
    SM_ASSERT_WIN32(GetClientRect(m_window, &rect));

    return rect;
}

bool Window::center_window(MultiMonitor monitor, bool topmost) {
    SM_UNUSED constexpr auto refl = ctu::reflect<MultiMonitor>();
    CTASSERTF(m_window != nullptr, "Window::center_window() called before Window::create()");
    CTASSERTF(monitor.is_valid(), "Window::center_window() invalid monitor: %s",
              refl.to_string(monitor).data());

    // get current monitor
    HMONITOR hmonitor = MonitorFromWindow(m_window, monitor.as_integral());
    if (!SM_CHECK_WIN32(hmonitor != nullptr, m_log)) return false;

    // get monitor info
    MONITORINFO monitor_info{.cbSize = sizeof(monitor_info)};
    if (!SM_CHECK_WIN32(GetMonitorInfoA(hmonitor, &monitor_info), m_log)) return false;

    // get window rect
    RECT rect{};
    if (!SM_CHECK_WIN32(GetWindowRect(m_window, &rect), m_log)) return false;

    // calculate center
    int x = (monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 -
            (rect.right - rect.left) / 2;
    int y = (monitor_info.rcWork.top + monitor_info.rcWork.bottom) / 2 -
            (rect.bottom - rect.top) / 2;

    HWND insert = topmost ? HWND_TOPMOST : HWND_TOP;
    UINT flags = topmost ? 0 : SWP_NOZORDER;

    // move window
    return SM_CHECK_WIN32(SetWindowPos(m_window, insert, x, y, 0, 0, SWP_NOSIZE | flags), m_log);
}
