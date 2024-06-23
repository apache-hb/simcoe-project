#include "stdafx.hpp"

#include "common.hpp"

#include "logs/logs.hpp"

using namespace sm;
using namespace sm::logs;

class ConsoleChannel final : public logs::ILogChannel {
    char mBuffer[2048];
    os::Console mConsole = os::Console::get();

    void acceptMessage(const logs::Message &message) noexcept override {
        size_t length = logs::buildMessageHeaderWithColour(mBuffer, message, kColourDefault);
        char *start = mBuffer + length;
        size_t remaining = sizeof(mBuffer) - length;

        logs::splitMessage(message.message, [&](std::string_view line) {
            auto [_, extra] = fmt::format_to_n(start, remaining, " {}\n", line);
            mConsole.print(std::string_view { mBuffer, static_cast<size_t>(length + extra) });
        });
    }

    void closeChannel() noexcept override {
        // nothing to do
    }

public:
    ConsoleChannel() noexcept = default;

    constexpr bool isHandleValid() const noexcept {
        return mConsole.valid();
    }
};

static ConsoleChannel gConsoleChannel;

bool logs::isConsoleHandleAvailable() noexcept {
    return gConsoleChannel.isHandleValid();
}

ILogChannel& logs::getConsoleHandle() noexcept {
    return gConsoleChannel;
}
