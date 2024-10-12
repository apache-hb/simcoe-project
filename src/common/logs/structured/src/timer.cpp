#include "stdafx.hpp"

#include "timer.hpp"

#include "core/win32.hpp"

namespace detail = sm::logs::structured::detail;
namespace chrono = std::chrono;

static uint64_t getCounterFrequency() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}

static const uint64_t kPerformanceCounterFrequency = getCounterFrequency();

detail::ITimeSource::ITimeSource() noexcept
    : mStartTime(chrono::system_clock::now())
{ }

/// high resoltuion timer based source
/// uses chrono::high_resolution_clock for portable high resolution time

detail::HighResolutionSource::HighResolutionSource() noexcept
    : mStartTicks(chrono::high_resolution_clock::now())
{ }

chrono::milliseconds detail::HighResolutionSource::getTimeSinceStart() const noexcept {
    PreciseTimePoint now = chrono::high_resolution_clock::now();
    chrono::milliseconds ms = chrono::duration_cast<chrono::milliseconds>(now - mStartTicks);
    return ms;
}

/// invariant TSC based source
/// uses rdtsc for better performance, but not portable or tested on all platforms

detail::InvariantTscSource::InvariantTscSource() noexcept
    : mStartTsc(__rdtsc())
{ }

chrono::milliseconds detail::InvariantTscSource::getTimeSinceStart() const noexcept {
    uint64_t tsc = __rdtsc();
    uint64_t ms = ((tsc - mStartTsc) / kPerformanceCounterFrequency);
    return chrono::milliseconds(ms);
}

static chrono::milliseconds detail::getCurrentTime(const ITimeSource& source) noexcept {
    chrono::milliseconds ms = source.getTimeSinceStart();
    SystemTimePoint time = source.getStartTime() + ms;

    return chrono::duration_cast<chrono::milliseconds>(time.time_since_epoch());
}
