#pragma once

#include <simcoe_config.h>

#include "system/system.hpp"

#include "math/math.hpp"
#include "core/macros.hpp"

namespace sm::sys {
    using WindowPlacement = WINDOWPLACEMENT;
    using Point = POINT;

    struct WindowCoords : RECT {
        constexpr WindowCoords(RECT rect) : RECT(rect) {}

        constexpr math::int2 size() const {
            return {right - left, bottom - top};
        }
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

        friend void create(HINSTANCE hInstance);

    public:
        SM_NOCOPY(Window)
        SM_NOMOVE(Window)

        Window(const WindowConfig &config, IWindowEvents& events);
        ~Window();

        WindowPlacement get_placement() const;
        WindowPlacement getPlacement() const { return get_placement(); }

        void set_placement(const WindowPlacement &placement);

        void show_window(ShowWindow show);

        void set_title(const char *title);

        bool center_window(MultiMonitor monitor = MultiMonitor{}, bool topmost = false);

        WindowCoords get_coords() const;
        WindowCoords get_client_coords() const;

        HWND get_handle() const { return mWindow; }
    };
}
