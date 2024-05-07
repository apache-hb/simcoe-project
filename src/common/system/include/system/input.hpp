#pragma once

#include "input/input.hpp"

#include "system/window.hpp"

namespace sm::sys {
    // mouse and keyboard input
    class DesktopInput final : public input::ISource {
        sys::Window& mWindow;

        // keyboard state
        input::ButtonState mButtons{};
        size_t mIndex = 0;

        math::int2 mMousePosition;
        bool mCaptureMouse = false;

        void setKeyValue(WORD key, size_t value);
        void setXButtonValue(WORD key, size_t value);

        bool pollMouseState(input::InputState& state);

        void centerMouse();

    public:
        DesktopInput(sys::Window& window);

        bool poll(input::InputState& state) override;

        void window_event(UINT msg, WPARAM wparam, LPARAM lparam);

        void capture_cursor(bool capture) override;
    };

    namespace mouse {
        void set_visible(bool visible);
    }
}
