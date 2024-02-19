#pragma once

namespace sm::sys {
    class Timer {
        size_t mStart;

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
