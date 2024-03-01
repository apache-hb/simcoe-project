#pragma once

#include <utility>

namespace sm {
    template<typename T>
    class WriteOnly {
        T *mData;

    public:
        constexpr WriteOnly(T *data = nullptr) : mData(data) {}

        constexpr WriteOnly& operator=(const T& value) {
            *mData = value;
            return *this;
        }

        constexpr WriteOnly& operator=(T&& value) {
            *mData = std::move(value);
            return *this;
        }

        constexpr T *operator&() {
            return mData;
        }
    };
}
