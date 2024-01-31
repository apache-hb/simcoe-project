#pragma once

#include "input/input.hpp"

#include "system/window.hpp"

namespace sm::sys {
    // mouse and keyboard input
    class DesktopInput : public input::ISource {
        // keyboard state
        input::ButtonState m_buttons{};
        size_t m_index = 0;

        math::int2 m_mouse_origin;
        math::int2 m_mouse_position;

        SM_UNUSED sys::Window& m_window;

        void set_key(WORD key, size_t value);
        void set_xbutton(WORD key, size_t value);

    public:
        DesktopInput(sys::Window& window);

        bool poll(input::InputState& state) override;

        void window_event(UINT msg, WPARAM wparam, LPARAM lparam);
    };
}
