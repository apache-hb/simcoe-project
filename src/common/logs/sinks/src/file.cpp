#include "stdafx.hpp"

#include "common.hpp"

#include "logs/structured/channel.hpp"
#include "logs/structured/logging.hpp"
#include "logs/structured/channels.hpp"

#include "core/fs.hpp"

#include <fstream>

namespace fs = sm::fs;
namespace os = sm::os;
namespace logs = sm::logs;
namespace structured = sm::logs::structured;

class FileChannel final : public structured::ILogChannel {
    char mBuffer[2048];

    std::ofstream mStream;
    std::mutex mMutex;

    void attach() override { }

    ~FileChannel() override {
        mStream.close();
    }

    void postMessage(structured::MessagePacket packet) noexcept override {
        auto message = ""; // fmt::vformat(packet.message.message, packet.args);

        std::lock_guard guard(mMutex);

        size_t length = logs::detail::buildMessageHeader(mBuffer, packet.timestamp, packet.message);
        char *start = mBuffer + length;
        size_t remaining = sizeof(mBuffer) - length;

        logs::detail::splitMessage(message, [&](auto line) {
            auto [_, extra] = fmt::format_to_n(start, remaining, " {}\n", line);
            mStream << std::string_view(mBuffer, length + extra);
        });
    }

public:
    FileChannel(const fs::path& path) noexcept
        : mStream(path, std::ios::out)
    { }
};

structured::ILogChannel *sm::logs::structured::file(const fs::path& path) {
    return new FileChannel(path);
}
