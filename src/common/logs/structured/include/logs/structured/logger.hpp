#pragma once

#include "logs/structured/channel.hpp"

namespace sm::logs::structured {
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
