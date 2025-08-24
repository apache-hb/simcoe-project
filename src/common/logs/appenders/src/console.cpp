#include "stdafx.hpp"

#include "common.hpp"

#include "logger/logging.hpp"
#include "logger/channel.hpp"
#include "logger/appenders/channels.hpp"

#if _WIN32
#   include "core/win32.hpp"
#endif

namespace os = sm::os;
namespace logs = sm::logs;
namespace appenders = sm::logs::appenders;

class ConsoleChannel final : public logs::ILogChannel {
    char mBuffer[0x1000 * 16];
    os::Console mConsole = os::Console::get();
    std::mutex mMutex;

    void attach() override { }

    void postMessage(logs::MessagePacket packet) noexcept override {
        auto message = fmt::vformat(packet.message.getMessage(), packet.args.asDynamicArgStore());

        std::lock_guard guard(mMutex);

        size_t length = logs::detail::buildMessageHeaderWithColour(mBuffer, packet.timestamp, packet.message, kColourDefault);
        char *start = mBuffer + length;
        size_t remaining = sizeof(mBuffer) - length;

        logs::detail::splitMessage(message, [&](std::string_view line) {
            auto [_, extra] = fmt::format_to_n(start, remaining, " {}\n", line);
            mConsole.print({ mBuffer, size_t(length + extra) });
        });
    }

public:
    ConsoleChannel() noexcept = default;
};

bool appenders::isConsoleAvailable() noexcept {
#if _WIN32
    return !!GetConsoleCP();
#else
    return true;
#endif
}

logs::ILogChannel *sm::logs::appenders::console() {
    return new ConsoleChannel;
}
