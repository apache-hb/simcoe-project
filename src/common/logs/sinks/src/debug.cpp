#include "stdafx.hpp"

#include "common.hpp"

#include "logs/sinks/channels.hpp"
#include "logs/logging.hpp"
#include "logs/message.hpp"

namespace os = sm::os;
namespace logs = sm::logs;
namespace sinks = sm::logs::sinks;

class DebugChannel final : public logs::ILogChannel {
    char mBuffer[2048];
    std::mutex mMutex;

    void attach() override { }

    void postMessage(logs::MessagePacket packet) noexcept override {
        auto message = ""; // fmt::vformat(packet.message.message, packet.args);

        std::lock_guard guard(mMutex);

        size_t length = logs::detail::buildMessageHeader(mBuffer, packet.timestamp, packet.message);
        char *start = mBuffer + length;
        size_t remaining = sizeof(mBuffer) - length;

        logs::detail::splitMessage(message, [&](auto line) {
            auto [_, extra] = fmt::format_to_n(start, remaining, " {}\n", line);
            os::printDebugMessage({ mBuffer, size_t(remaining - extra) });
        });
    }
};

bool sinks::isDebugConsoleAvailable() noexcept {
    return os::isDebuggerAttached();
}

logs::ILogChannel *sinks::debugConsole() {
    return new DebugChannel;
}
