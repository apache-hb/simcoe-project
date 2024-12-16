#pragma once

#include <simcoe_config.h>

#include "base/macros.hpp"

#include "system/system.hpp"

#include "math/math.hpp"

namespace sm::system {
    enum class ShowWindow {
        eHide = SW_HIDE,
        eShowNormal = SW_SHOWNORMAL,
        eShow = SW_SHOW,
        eRestore = SW_RESTORE,
        eShowDefault = SW_SHOWDEFAULT,
    };

    enum class MultiMonitor {
        eNull = MONITOR_DEFAULTTONULL,
        ePrimary = MONITOR_DEFAULTTOPRIMARY,
        eNearest = MONITOR_DEFAULTTONEAREST,
    };

    enum class WindowMode : DWORD {
        eWindowed = WS_OVERLAPPEDWINDOW,
        eBorderless = WS_POPUP,
    };

    using WindowPlacement = WINDOWPLACEMENT;
    using Point = POINT;

    struct WindowCoords : RECT {
        constexpr WindowCoords(RECT rect) : RECT(rect) {}

        constexpr math::int2 size() const {
            return {right - left, bottom - top};
        }
    };

    struct WindowConfig {
        WindowMode mode;
        int width;
        int height;
        std::string title;
    };

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
        HWND mWindow = nullptr;
        IWindowEvents& mEvents;

        static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

        void create(const WindowConfig &config);
        void destroy_window();
        void destroyWindow() { destroy_window(); }

        friend void create(HINSTANCE hInstance);

    public:
        SM_NOCOPY(Window)
        SM_NOMOVE(Window)

        Window(const WindowConfig &config, IWindowEvents& events);
        ~Window();

        WindowPlacement getPlacement() const;

        void setPlacement(const WindowPlacement &placement);

        void show_window(ShowWindow show);
        void showWindow(ShowWindow show) { show_window(show); }

        void resize(math::int2 size);

        void set_title(const char *title);
        void setTitle(const char *title) { set_title(title); }

        bool center_window(MultiMonitor monitor = MultiMonitor{}, bool topmost = false);
        bool centerWindow(MultiMonitor monitor = MultiMonitor{}, bool topmost = false) {
            return center_window(monitor, topmost);
        }

        WindowCoords get_coords() const;
        WindowCoords get_client_coords() const;

        WindowCoords getCoords() const { return get_coords(); }
        WindowCoords getClientCoords() const { return get_client_coords(); }

        math::uint2 getClientSize() const { return getClientCoords().size(); }

        HWND get_handle() const { return mWindow; }
        HWND getHandle() const { return get_handle(); }
    };
}
