#include "stdafx.hpp"

#include "common.hpp"

#include "logs/logs.hpp"

using namespace sm;
using namespace sm::logs;

class DebugChannel final : public logs::ILogChannel {
    char mBuffer[2048];

    void acceptMessage(const logs::Message &message) noexcept override {
        size_t length = logs::buildMessageHeader(mBuffer, message);
        char *start = mBuffer + length;
        size_t remaining = sizeof(mBuffer) - length;

        for (sm::StringView line : logs::splitMessage(message.message)) {
            fmt::format_to_n(start, remaining, " {}\n", line);
            OutputDebugStringA(mBuffer);
        }
    }

    void closeChannel() noexcept override {
        // nothing to do
    }
};

bool logs::isDebugConsoleAvailable() noexcept {
    return IsDebuggerPresent();
}

ILogChannel& logs::getDebugConsole() noexcept {
    static DebugChannel instance{};
    return instance;
}
