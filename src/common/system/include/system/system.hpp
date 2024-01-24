#pragma once

#include "core/macros.hpp"
#include "base/panic.h"

#include "system.reflect.h"

typedef struct arena_t arena_t;

namespace sm { class IArena; }

#define SM_ASSERT_WIN32(expr)                         \
    do {                                              \
        if (auto result = (expr); !result) {          \
            sm::sys::assert_last_error(CT_SOURCE_HERE, #expr); \
        }                                             \
    } while (0)

// like assert but non-fatal if the api call fails
#define SM_CHECK_WIN32(expr, sink)                             \
    [&]() -> bool {                                            \
        if (auto result = (expr); !result) {                   \
            (sink).error(#expr " = {}. {}", result, sm::sys::get_last_error()); \
            return false;                                      \
        }                                                      \
        return true;                                           \
    }()

namespace sm::sys {
    using SystemSink = logs::Sink<logs::Category::eSystem>;
    using WindowPlacement = WINDOWPLACEMENT;
    using Point = POINT;
    using WindowCoords = RECT;

    NORETURN
    assert_last_error(source_info_t panic, const char *expr);

    char *get_last_error(void);

    class IWindowEvents {
        friend class Window;

    protected:
        virtual ~IWindowEvents() = default;

        virtual void create(Window& window) = 0;
        virtual void destroy(Window& window) = 0;
        virtual bool close(Window& window) = 0;
    };

    class Window {
        HWND m_window = nullptr;
        IWindowEvents *m_events = nullptr;
        SystemSink m_log;

        static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);

        void create(const WindowConfig& config);

        friend void create(HINSTANCE hInstance, logs::ILogger& logger);

    public:
        SM_NOCOPY(Window)
        SM_NOMOVE(Window)

        Window(const WindowConfig& config, IWindowEvents *events);
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

    void create(HINSTANCE hInstance, logs::ILogger& logger);
    void destroy(void);
}
