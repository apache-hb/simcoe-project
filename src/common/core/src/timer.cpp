#include "core/timer.hpp"

using namespace sm;

Timer::Timer()
    : mStart(std::chrono::steady_clock::now())
{ }

float Timer::elapsed() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(now - mStart).count();
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
