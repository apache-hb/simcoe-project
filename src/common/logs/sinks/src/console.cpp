#include "stdafx.hpp"

#include "logs/structured/logging.hpp"
#include "logs/structured/channel.hpp"
#include "logs/structured/channels.hpp"

namespace structured = sm::logs::structured;

class ConsoleChannel final : public structured::ILogChannel {
    std::unordered_map<uint64_t, const structured::MessageInfo*> mMessages;

    void attach() override {
        auto config = structured::getMessages();
        for (const auto& message : config.messages) {
            mMessages[message.hash] = &message;
        }
    }

    void postMessage(structured::MessagePacket packet) noexcept override {
        uint64_t hash = packet.message.hash;
        if (!mMessages.contains(hash)) {
            fmt::println(stderr, "Unknown message: {}", hash);
            return;
        }

        auto it = mMessages.at(hash);

        fmt::vprintln(stderr, it->message, packet.args);
    }

public:
    ConsoleChannel() noexcept = default;
};

structured::ILogChannel *sm::logs::structured::console() {
    return new ConsoleChannel;
}
