#include "stdafx.hpp"

#include "common.hpp"

#include "logs/structured/channels.hpp"
#include "logs/structured/logging.hpp"
#include "logs/structured/message.hpp"

using namespace sm;
namespace structured = sm::logs::structured;

class DebugChannel final : public structured::ILogChannel {
    char mBuffer[2048];
    std::mutex mMutex;

    void attach() override { }

    void postMessage(structured::MessagePacket packet) noexcept override {
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

bool structured::isDebugConsoleAvailable() noexcept {
    return os::isDebuggerAttached();
}

structured::ILogChannel *structured::debugConsole() {
    return new DebugChannel;
}
