#include "stdafx.hpp"

#include "timer.hpp"

#if CT_HAS_TSC_TIMESOURCE
#   include "core/win32.hpp"
#endif

namespace detail = sm::logs::detail;
namespace chrono = std::chrono;

#if CT_HAS_TSC_TIMESOURCE
static uint64_t getCounterFrequency() {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}

static const uint64_t kPerformanceCounterFrequency = getCounterFrequency();
#endif

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

#if CT_HAS_TSC_TIMESOURCE
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
#endif
