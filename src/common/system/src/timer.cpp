#include "system/timer.hpp"

#include "common.hpp"

using namespace sm;
using namespace sm::sys;

Timer::Timer()
    : mStart(query_timer())
{ }

float Timer::elapsed() const {
    size_t now = query_timer();
    return float(now - mStart) / float(gTimerFrequency);
}

Ticker::Ticker()
    : Timer()
    , mLastUpdate(elapsed())
{ }

float Ticker::tick() {
    float now = elapsed();
    float delta = now - mLastUpdate;
    mLastUpdate = now;
    return delta;
}
