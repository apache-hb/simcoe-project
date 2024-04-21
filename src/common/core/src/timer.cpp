#include "core/timer.hpp"

using namespace sm;

Timer::Timer() noexcept
    : mStart(std::chrono::steady_clock::now())
{ }

float Timer::elapsed() const noexcept {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<float>(now - mStart).count();
}

Ticker::Ticker() noexcept
    : mLastUpdate(elapsed())
{ }

float Ticker::tick() noexcept {
    float now = elapsed();
    float delta = now - mLastUpdate;
    mLastUpdate = now;
    return delta;
}
