#include "stdafx.hpp"

#include "logs/structured/logger.hpp"

namespace structured = sm::logs::structured;

using MessageInfo = sm::logs::structured::MessageInfo;

// theres probably like 2 or 3 ticks of difference between these
// but it's not really a big deal
static const auto kStartTime = std::chrono::system_clock::now();
static const auto kStartTicks = std::chrono::high_resolution_clock::now();

static uint64_t getTimestamp() noexcept {
    // use kStartTime and kStartTicks to calculate current time in milliseconds
    // using high_resolution_clock is much faster than system_clock on windows
    auto now = std::chrono::high_resolution_clock::now();
    auto ticksSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(now - kStartTicks);
    auto timeSinceStart = kStartTime + ticksSinceStart;

    return std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceStart.time_since_epoch()).count();
}

void structured::Logger::addChannel(std::unique_ptr<ILogChannel> channel) {
    channel->attach();
    mChannels.emplace_back(std::move(channel));
}

void structured::Logger::shutdown() noexcept {
    mChannels.clear();
}

void structured::Logger::postMessage(const MessageInfo& message, ArgStore args) noexcept {
    uint64_t timestamp = getTimestamp();
    std::shared_ptr<ArgStore> sharedArgs = std::make_shared<ArgStore>(std::move(args));

    for (auto& channel : mChannels) {
        LogMessagePacket packet = {
            .message = message,
            .timestamp = timestamp,
            .args = sharedArgs
        };
        channel->postMessage(packet);
    }
}

structured::Logger& structured::Logger::instance() noexcept {
    static Logger sInstance;
    return sInstance;
}
