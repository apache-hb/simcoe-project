#pragma once

#include "input/input.hpp"

#include "system/window.hpp"

namespace sm::sys {
    // mouse and keyboard input
    class DesktopInput : public input::ISource {
        sys::Window& mWindow;

        // keyboard state
        input::ButtonState mButtons{};
        size_t mIndex = 0;

        math::int2 mMousePosition;

        void set_key(WORD key, size_t value);
        void set_xbutton(WORD key, size_t value);

        bool poll_mouse(input::InputState& state);

    public:
        DesktopInput(sys::Window& window);

        bool poll(input::InputState& state) override;

        void window_event(UINT msg, WPARAM wparam, LPARAM lparam);
    };
}
