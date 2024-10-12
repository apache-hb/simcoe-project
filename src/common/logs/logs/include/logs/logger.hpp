#pragma once

#include "logs/channel.hpp"

namespace sm::logs {
    enum class TimerSource {
        eAutoDetect, // defaults to invariant TSC if available, otherwise high resolution clock

        eHighResolutionClock, // force high resolution clock
        eInvariantTsc, // force invariant TSC, not recommended
    };

    struct LoggingConfig {
        TimerSource timer = TimerSource::eAutoDetect;
    };

    void create(LoggingConfig config = {});

    class Logger {
        std::vector<std::unique_ptr<ILogChannel>> mChannels;
        std::unique_ptr<IAsyncLogChannel> mAsyncChannel;

    public:
        void addChannel(std::unique_ptr<ILogChannel>&& channel);
        void setAsyncChannel(std::unique_ptr<IAsyncLogChannel>&& channel);

        void destroy() noexcept;

        void postMessage(const MessageInfo& message, std::unique_ptr<DynamicArgStore> args) noexcept;

        static Logger& instance() noexcept;
    };
}
