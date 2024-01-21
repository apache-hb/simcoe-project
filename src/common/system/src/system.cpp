#include "system/system.hpp"

#include "common.hpp"

#include "base/panic.h"

#include "resource.h"

using namespace sm;
using namespace sm::sys;

#define SM_CLASS_NAME "simcoe"

static HINSTANCE gInstance = nullptr;
static LPTSTR gWindowClass = nullptr;

static DWORD get_window_style(WindowMode mode) {
    switch (mode) {
    case WindowMode::eBorderless: return WS_POPUP;
    case WindowMode::eWindowed: return WS_OVERLAPPEDWINDOW;
    default: NEVER("invalid window mode: %d", mode.as_integral());
    }
}

LRESULT CALLBACK Window::proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    switch (message) {
    case WM_CREATE: {
        CREATESTRUCT *create = reinterpret_cast<CREATESTRUCT*>(lparam);
        Window *self = reinterpret_cast<Window*>(create->lpCreateParams);

        // CreateWindow calls WM_CREATE before returning
        // so we need to set the window handle here
        self->m_window = window;

        SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));

        self->m_events->create(*self);
        break;
    }
    case WM_CLOSE: {
        Window *self = reinterpret_cast<Window*>(GetWindowLongPtrA(window, GWLP_USERDATA));
        if (self == nullptr)
            DestroyWindow(window);
        else if (self->m_events->close(*self))
            DestroyWindow(window);

        break;
    }
    case WM_DESTROY: {
        Window *self = reinterpret_cast<Window*>(GetWindowLongPtrA(window, GWLP_USERDATA));
        if (self != nullptr) self->m_events->destroy(*self);

        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProcA(window, message, wparam, lparam);
    }

    return 0;
}

void Window::create(const WindowInfo& info) {
    CTASSERTF(gInstance != nullptr, "system::create() not called before Window::create()");
    CTASSERTF(gWindowClass != nullptr, "system::create() not called before Window::create()");

    m_window = CreateWindowA(
        /* lpClassName = */ gWindowClass,
        /* lpWindowName = */ info.title,
        /* dwStyle = */ get_window_style(info.mode),
        /* x = */ CW_USEDEFAULT,
        /* y = */ CW_USEDEFAULT,
        /* nWidth = */ info.width,
        /* nHeight = */ info.height,
        /* hWndParent = */ nullptr,
        /* hMenu = */ nullptr,
        /* hInstance = */ gInstance,
        /* lpParam = */ this
    );

    SM_ASSERT_WIN32(m_window != nullptr);
}

Window::Window(const WindowInfo& info, IWindowEvents *events) : m_events(events) {
    CTASSERT(events != nullptr);

    create(info);
}

Window::~Window() {
    if (m_window != nullptr)
        destroy_window();
}

WindowPlacement Window::get_placement(void) const {
    WINDOWPLACEMENT placement{ .length = sizeof(WINDOWPLACEMENT) };
    SM_ASSERT_WIN32(GetWindowPlacement(m_window, &placement));

    return placement;
}

void Window::set_placement(const WindowPlacement& placement) {
    SM_ASSERT_WIN32(SetWindowPlacement(m_window, &placement));
}

void Window::show_window(ShowWindow show) {
    CTASSERTF(m_window != nullptr, "Window::show_window() called before Window::create()");
    CTASSERTF(show.is_valid(), "Window::show_window() invalid show: %d", show.as_integral());
    ::ShowWindow(m_window, show.as_integral());
}

void Window::destroy_window(void) {
    SM_ASSERT_WIN32(DestroyWindow(m_window));
    m_window = nullptr;
}

void Window::set_title(const char *title) {
    CTASSERT(title != nullptr);

    SM_ASSERT_WIN32(SetWindowTextA(m_window, title));
}

WindowCoords Window::get_coords() const {
    RECT rect{};
    SM_ASSERT_WIN32(GetWindowRect(m_window, &rect));

    WindowCoords coords = {
        .left = rect.left,
        .top = rect.top,
        .right = rect.right,
        .bottom = rect.bottom,
    };

    return coords;
}

bool Window::center_window(MultiMonitor monitor) {
    constexpr auto refl = ctu::reflect<MultiMonitor>();
    CTASSERTF(m_window != nullptr, "Window::center_window() called before Window::create()");
    CTASSERTF(monitor.is_valid(), "Window::center_window() invalid monitor: %s", refl.to_string(monitor).data());

    // get current monitor
    HMONITOR hmonitor = MonitorFromWindow(m_window, monitor.as_integral());
    SM_ASSERT_WIN32(hmonitor != nullptr);

    // get monitor info
    MONITORINFO monitor_info { .cbSize = sizeof(monitor_info) };
    SM_ASSERT_WIN32(GetMonitorInfoA(hmonitor, &monitor_info));

    // get window rect
    RECT rect{};
    SM_ASSERT_WIN32(GetWindowRect(m_window, &rect));

    // calculate center
    int x = (monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 - (rect.right - rect.left) / 2;
    int y = (monitor_info.rcWork.top + monitor_info.rcWork.bottom) / 2 - (rect.bottom - rect.top) / 2;

    // move window
    SM_ASSERT_WIN32(SetWindowPos(m_window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER));

    return true;
}

void sys::create(HINSTANCE hInstance) {
    CTASSERTF(hInstance != nullptr, "system::create() invalid hInstance");
    CTASSERTF(gWindowClass == nullptr, "system::create() called twice");

    gInstance = hInstance;

    HICON hIcon = LoadIconA(
        /* hInst = */ hInstance,
        /* name = */ MAKEINTRESOURCEA(IDI_DEFAULT_ICON));

    SM_ASSERT_WIN32(hIcon != nullptr);

    const WNDCLASSEXA kClass = {
        .cbSize = sizeof(WNDCLASSEX),

        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = Window::proc,
        .hInstance = hInstance,
        .hIcon = hIcon,
        .lpszClassName = SM_CLASS_NAME
    };

    if (ATOM atom = RegisterClassExA(&kClass); atom == 0) {
        assert_last_error(CTU_PANIC_INFO, "RegisterClassExA");
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
