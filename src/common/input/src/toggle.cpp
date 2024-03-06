#include "input/toggle.hpp"

using namespace sm;
using namespace sm::input;

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
