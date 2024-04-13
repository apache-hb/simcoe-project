#pragma once

#include "input/input.hpp"

namespace sm::input {
    class Debounce {
        Button mButton;
        size_t mAcceptedValue = 0;

    public:
        Debounce(Button button)
            : mButton(button)
        { }

        bool is_pressed(const InputState& state) {
            if (state.buttons[mButton] == 0) {
                mAcceptedValue = 0;
                return false;
            }

            if (state.buttons[mButton] != mAcceptedValue) {
                mAcceptedValue = state.buttons[mButton];
                return true;
            }

            return false;
        }
    };
}
