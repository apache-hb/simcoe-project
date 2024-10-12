#pragma once

#include <chrono>

namespace sm::logs::structured::detail {
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

    class InvariantTscSource final : public ITimeSource {
        const uint64_t mStartTsc;
    public:
        InvariantTscSource() noexcept;

        std::chrono::milliseconds getTimeSinceStart() const noexcept;
    };

    std::chrono::milliseconds getCurrentTime(const ITimeSource& source) noexcept;
}
