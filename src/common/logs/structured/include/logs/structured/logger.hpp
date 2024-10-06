#pragma once

#include "logs/structured/channel.hpp"

namespace sm::logs::structured {
    class Logger {
        std::vector<std::unique_ptr<ILogChannel>> mChannels;

    public:
        void addChannel(std::unique_ptr<ILogChannel>&& channel);

        void destroy() noexcept;

        void postMessage(const MessageInfo& message, ArgStore args) noexcept;

        static Logger& instance() noexcept;
    };
}
