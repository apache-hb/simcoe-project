#include "stdafx.hpp"

#include "common.hpp"

#include "logs/logs.hpp"

using namespace sm;
using namespace sm::logs;

class ConsoleChannel final : public logs::ILogChannel {
    char mBuffer[2048];
    HANDLE mConsoleHandle;

    void accept(const logs::Message &message) noexcept override {
        size_t length = logs::buildMessageHeaderWithColour(mBuffer, message, kColourDefault);
        char *start = mBuffer + length;
        size_t remaining = sizeof(mBuffer) - length;

        for (sm::StringView line : logs::splitMessage(message.message)) {
            auto [_, extra] = fmt::format_to_n(start, remaining, " {}\n", line);
            DWORD written;
            WriteConsoleA(mConsoleHandle, mBuffer, static_cast<DWORD>(length + extra), &written, nullptr);
        }
    }

public:
    ConsoleChannel() noexcept {
        mConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    constexpr bool isHandleValid() const noexcept {
        return mConsoleHandle != INVALID_HANDLE_VALUE;
    }
};

static ConsoleChannel gConsoleChannel;

bool logs::isConsoleHandleAvailable() noexcept {
    return gConsoleChannel.isHandleValid();
}

ILogChannel& logs::getConsoleHandle() noexcept {
    return gConsoleChannel;
}
