#pragma once

#include <chrono>

namespace sm {
    class Timer {
        std::chrono::steady_clock::time_point mStart;

    public:
        Timer();

        float elapsed() const;
    };

    class Ticker : Timer {
        float mLastUpdate;

    public:
        Ticker();

        float tick();
    };
}
