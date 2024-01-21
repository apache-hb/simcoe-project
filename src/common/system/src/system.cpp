#include "system/system.hpp"

#include "common.hpp"

#include "core/arena.hpp"

#include "base/panic.h"
#include "io/io.h"
#include "os/os.h"

#include "resource.h"

using namespace sm;
using namespace sm::system;

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
    // if i ever handle WM_CLOSE, update Window::destroy() to call DestroyWindow()
    switch (message) {
    case WM_CREATE: {
        CREATESTRUCT *create = reinterpret_cast<CREATESTRUCT*>(lparam);
        Window *self = reinterpret_cast<Window*>(create->lpCreateParams);

        SetWindowLongPtrA(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));

        self->m_events->create(*self);
        break;
    }
    case WM_CLOSE: {
        Window *self = reinterpret_cast<Window*>(GetWindowLongPtrA(window, GWLP_USERDATA));
        if (self != nullptr) {
            if (self->m_events->close(*self)) {
                DestroyWindow(window);
            }
        } else {
            DestroyWindow(window);
        }
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

    // if we have placement data, use that instead of the default
    io_t *io = io_file(SM_CLASS_NAME ".placement.bin", eAccessRead, sm::get_debug_arena());
    if (io_error_t err = io_error(io); err == 0) {
        WINDOWPLACEMENT placement{};
        io_read(io, &placement, sizeof(placement));
        io_close(io);

        SM_ASSERT_WIN32(SetWindowPlacement(m_window, &placement));
    }
    // no placement data and centering requested, so center the window
    else if (info.center) {

        // get current monitor
        HMONITOR monitor = MonitorFromWindow(m_window, MONITOR_DEFAULTTONEAREST);
        SM_ASSERT_WIN32(monitor != nullptr);

        // get monitor info
        MONITORINFO monitor_info { .cbSize = sizeof(monitor_info) };
        SM_ASSERT_WIN32(GetMonitorInfoA(monitor, &monitor_info));

        // get window rect
        RECT rect{};
        SM_ASSERT_WIN32(GetWindowRect(m_window, &rect));

        // calculate center
        int x = (monitor_info.rcWork.left + monitor_info.rcWork.right) / 2 - (rect.right - rect.left) / 2;
        int y = (monitor_info.rcWork.top + monitor_info.rcWork.bottom) / 2 - (rect.bottom - rect.top) / 2;

        // move window
        SM_ASSERT_WIN32(SetWindowPos(m_window, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER));
    }

    io_close(io);
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
    WINDOWPLACEMENT placement{};
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

void system::create(HINSTANCE hInstance) {
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

void system::destroy(void) {
    CTASSERTF(gInstance != nullptr, "system::destroy() called before system::create()");
    CTASSERTF(gWindowClass != nullptr, "system::destroy() called before system::create()");

    SM_ASSERT_WIN32(UnregisterClassA(gWindowClass, gInstance));

    gInstance = nullptr;
    gWindowClass = nullptr;
}
