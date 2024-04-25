#pragma once

namespace sm::input {
    template<typename T>
    struct DeltaAxis {
        T mLastValue = 0.f;

    public:
        T getNewDelta(T value) {
            T delta = value - mLastValue;
            mLastValue = value;
            return delta;
        }
    };
}
