#include "input/toggle.hpp"
#include "input/debounce.hpp"

using namespace sm;
using namespace sm::input;

// only accept a new value if it is greater than the last value

bool Debounce::isPressed(const InputState& state) {
    if (state.buttons[std::to_underlying(mButton)] == 0) {
        mAcceptedValue = 0;
        return false;
    }

    if (state.buttons[std::to_underlying(mButton)] != mAcceptedValue) {
        mAcceptedValue = state.buttons[std::to_underlying(mButton)];
        return true;
    }

    return false;
}

// toggle button

Toggle::Toggle(bool initial)
    : mActive(initial)
{ }

bool Toggle::update(size_t value) {
    if (value > mLastValue) {
        mLastValue = value;
        mActive = !mActive;
        return true;
    }

    return false;
}

bool Toggle::is_active() const {
    return mActive;
}

void Toggle::activate(bool active) {
    mLastValue = 0;
    mActive = active;
}
