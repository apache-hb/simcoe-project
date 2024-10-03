#pragma once

#include "logs/structured/channel.hpp"

namespace sm::logs::structured {
    class Logger {
        std::vector<std::unique_ptr<ILogChannel>> mChannels;
        std::unique_ptr<IAsyncLogChannel> mAsyncChannel;

    public:
        void addChannel(std::unique_ptr<ILogChannel> channel);
        void addAsyncChannel(std::unique_ptr<IAsyncLogChannel> channel);

        void shutdown() noexcept;

        void postMessage(const MessageInfo& message, ArgStore args) noexcept;

        static Logger& instance() noexcept;
    };
}
