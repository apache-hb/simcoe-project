#include "stdafx.hpp"

#include "logs/structured/category.hpp"
#include "logs/structured/channel.hpp"

using namespace sm::logs;
using namespace sm::logs::structured;

class ConsoleChannel final : public structured::ILogChannel {
    std::unordered_map<uint64_t, const LogMessageInfo*> mMessages;

    void attach(
        std::span<const Category> categories,
        std::span<const LogMessageInfo> messages
    ) noexcept override {
        for (const auto& message : messages) {
            mMessages[message.hash] = &message;
        }
    }

    void postMessage(const LogMessageInfo& message, fmt::format_args args) noexcept override {
        auto it = mMessages.find(message.hash);
        if (it == mMessages.end()) {
            fmt::println(stderr, "Unknown message: {}", message.hash);
            return;
        }

        fmt::vprintln(stderr, message.message, args);
    }

public:
    ConsoleChannel() noexcept = default;
};

structured::ILogChannel *sm::logs::structured::console() noexcept {
    static ConsoleChannel channel;
    return &channel;
}
