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

        bool isPressed(const InputState& state);
    };
}
