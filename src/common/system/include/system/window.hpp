#pragma once

#include <simcoe_config.h>

#include "db/connection.hpp"
#include "system.dao.hpp"

#include "system/system.hpp"

#include "math/math.hpp"
#include "core/macros.hpp"

#if _WIN32
#   define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace sm::system {
    enum class ShowWindow {
        eHide,
        eShowNormal,
        eShow,
        eRestore,
        eShowDefault,
    };

    enum class MultiMonitor {
        eNearest, ///< center on whichever monitors center is nearest to the window
        ePrimary, ///< center on the primary monitor
    };

    enum class WindowMode {
        eWindowed,
        eBorderless,
    };

    struct WindowPlacement {
        math::int2 size;
        math::int2 position;

        math::int2 center() const noexcept {
            return (position + size) / 2;
        }
    };

    struct WindowConfig {
        WindowMode mode;
        int width;
        int height;
        std::string title;

        math::int2 minSize = { 64, 64 };
        math::int2 maxSize = { 4096, 4096 };
    };

    class IWindowEvents {
        friend class Window;

    protected:
        virtual ~IWindowEvents() = default;

        virtual void resize(Window &window, math::int2 size) {}
        virtual void create(Window &window) {}
        virtual void destroy(Window &window) {}
        virtual bool close(Window &window) {
            return true;
        }
    };

    using WindowHandle = std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)>;

    class Window {
        WindowHandle mWindow;
        std::string mTitle;
        IWindowEvents& mEvents;

    public:
        Window(const WindowConfig &config, IWindowEvents& events);

        math::int2 getPosition() const;
        void setPosition(math::int2 position);

        math::int2 getSize() const;
        void setSize(math::int2 size);

        WindowPlacement getPlacement() const;
        void setPlacement(WindowPlacement placement);

        bool shouldClose() const;
        void pollEvents();

        void showWindow(ShowWindow show);

        void setTitle(const std::string& title);
        std::string getTitle() const;

        void centerWindow(MultiMonitor monitor = MultiMonitor::eNearest, bool topmost = false);

        math::uint2 getClientSize() const { return getPlacement().size; }

        HWND getHandle() const { return glfwGetWin32Window(mWindow.get()); }
        GLFWwindow* get() const { return mWindow.get(); }
    };

    uint64_t saveWindowInfo(db::Connection& db, const Window& window);
    std::optional<dao::system::WindowInfo> loadWindowInfo(db::Connection& db, uint64_t id);
    void applyWindowInfo(Window& window, const dao::system::WindowInfo& info);
}
