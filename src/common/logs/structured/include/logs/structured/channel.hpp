#pragma once

#include "core/adt/small_vector.hpp"

#include "logs/structured/core.hpp"

namespace sm::logs::structured {
    class ILogChannel {
    public:
        virtual ~ILogChannel() = default;

        virtual void attach() noexcept = 0;

        virtual void postMessage(const LogMessageInfo& message, sm::SmallVectorBase<std::string> args) noexcept = 0;
    };

    class Logger {
        std::vector<ILogChannel*> mChannels;

    public:
        void addChannel(ILogChannel& channel) noexcept;

        void postMessage(const LogMessageInfo& message, sm::SmallVectorBase<std::string> args) noexcept;
    };
}
