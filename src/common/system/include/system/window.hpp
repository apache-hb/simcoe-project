#pragma once

#include <simcoe_config.h>

#include "system/system.hpp"

#include "math/math.hpp"
#include "core/macros.hpp"

namespace sm::sys {
using WindowPlacement = WINDOWPLACEMENT;
using Point = POINT;
using WindowCoords = RECT;

class IWindowEvents {
    friend class Window;

protected:
    virtual ~IWindowEvents() = default;

    virtual LRESULT event(Window &window, UINT message, WPARAM wparam, LPARAM lparam) {
        return 0;
    }

    virtual void resize(Window &window, math::int2 size) {}
    virtual void create(Window &window) {}
    virtual void destroy(Window &window) {}
    virtual bool close(Window &window) {
        return true;
    }
};

class Window {
    HWND m_window = nullptr;
    IWindowEvents *m_events = nullptr;
    SystemSink m_log;

    static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

    void create(const WindowConfig &config);
    void destroy_window();

    friend void create(HINSTANCE hInstance, logs::ILogger &logger);

public:
    SM_NOCOPY(Window)
    SM_NOMOVE(Window)

    Window(const WindowConfig &config, IWindowEvents *events);
    ~Window();

    WindowPlacement get_placement() const;
    void set_placement(const WindowPlacement &placement);

    void show_window(ShowWindow show);

    void set_title(const char *title);

    bool center_window(MultiMonitor monitor = MultiMonitor{}, bool topmost = false);

    WindowCoords get_coords() const;
    WindowCoords get_client_coords() const;

    HWND get_handle() const {
        return m_window;
    }
};
}