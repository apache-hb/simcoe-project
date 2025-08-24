#pragma once

#include <chrono>

#include "core/compiler.h" // IWYU pragma: keep - defines CT_OS_WINDOWS

#if defined(__x86_64__) && defined(CT_OS_WINDOWS)
#   define CT_HAS_TSC_TIMESOURCE 1
#else
#   define CT_HAS_TSC_TIMESOURCE 0
#endif

namespace sm::logs::detail {
    using SystemTimePoint = std::chrono::system_clock::time_point;
    using PreciseTimePoint = std::chrono::high_resolution_clock::time_point;

    class ITimeSource {
        const SystemTimePoint mStartTime;
    public:
        ITimeSource() noexcept;

        SystemTimePoint getStartTime() const noexcept { return mStartTime; }

        virtual ~ITimeSource() = default;

        virtual std::chrono::milliseconds getTimeSinceStart() const noexcept = 0;
    };

    class HighResolutionSource final : public ITimeSource {
        const PreciseTimePoint mStartTicks;
    public:
        HighResolutionSource() noexcept;

        std::chrono::milliseconds getTimeSinceStart() const noexcept;
    };

#if CT_HAS_TSC_TIMESOURCE
    class InvariantTscSource final : public ITimeSource {
        const uint64_t mStartTsc;
    public:
        InvariantTscSource() noexcept;

        std::chrono::milliseconds getTimeSinceStart() const noexcept;
    };
#endif

    std::chrono::milliseconds getCurrentTime(const ITimeSource& source) noexcept;
}
