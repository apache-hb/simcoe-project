#pragma once

#include <sm_system_api.hpp>

#include "core/win32.h" // IWYU pragma: export

#include "system.reflect.h"

typedef struct arena_t arena_t;

namespace sm { class IArena; }

namespace sm::sys {
    using WindowPlacement = WINDOWPLACEMENT;
    using Point = POINT;
    using WindowCoords = RECT;

    class SM_SYSTEM_API IWindowEvents {
        friend class Window;

    protected:
        virtual ~IWindowEvents() = default;

        virtual void create(Window& window) = 0;
        virtual void destroy(Window& window) = 0;
        virtual bool close(Window& window) = 0;
    };

    class SM_SYSTEM_API Window {
        HWND m_window = nullptr;
        IWindowEvents *m_events = nullptr;

        static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

        void create(const WindowInfo& info);

        friend void create(HINSTANCE hInstance);

    public:
        SM_NOCOPY(Window)

        Window(const WindowInfo& info, IWindowEvents *events);
        ~Window();

        WindowPlacement get_placement(void) const;
        void set_placement(const WindowPlacement& placement);

        void show_window(ShowWindow show);

        void destroy_window(void);

        void set_title(const char *title);

        bool center_window(MultiMonitor monitor = MultiMonitor());

        WindowCoords get_coords() const;

        HWND get_handle() const { return m_window; }
    };

    void create(HINSTANCE hInstance);
    void destroy(void);
}
