#pragma once

#include "logs/structured/message.hpp"

namespace sm::logs::structured {
    struct MessagePacket {
        const MessageInfo& message;
        uint64_t timestamp;
        std::shared_ptr<ArgStore> args;
    };

    class ILogChannel {
    public:
        virtual ~ILogChannel() = default;

        virtual void attach() = 0;

        virtual void postMessage(MessagePacket packet) noexcept = 0;
    };
}
