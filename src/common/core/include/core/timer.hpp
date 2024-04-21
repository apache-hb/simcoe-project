#pragma once

#include <chrono>

namespace sm {
    class Timer {
        std::chrono::steady_clock::time_point mStart;

    public:
        Timer() noexcept;

        float elapsed() const noexcept;
    };

    class Ticker : Timer {
        float mLastUpdate;

    public:
        Ticker() noexcept;

        float tick() noexcept;
    };
}
