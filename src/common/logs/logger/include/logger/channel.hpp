#pragma once

#include "logger/message.hpp"

namespace sm::logs {
    struct MessagePacket {
        const MessageInfo& message;
        uint64_t timestamp;
        const DynamicArgStore& args;
    };

    struct AsyncMessagePacket {
        const MessageInfo& message;
        uint64_t timestamp;
        std::unique_ptr<DynamicArgStore> args;
    };

    class ILogChannel {
    public:
        virtual ~ILogChannel() = default;

        virtual void attach() = 0;

        virtual void postMessage(MessagePacket packet) noexcept = 0;
    };

    class IAsyncLogChannel : public ILogChannel {
        void postMessage(MessagePacket packet) noexcept override { }

    public:
        virtual void postMessageAsync(AsyncMessagePacket packet) noexcept = 0;
    };
}
