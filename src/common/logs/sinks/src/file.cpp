#include "stdafx.hpp"

#include "logs/structured/channel.hpp"
#include "logs/structured/logging.hpp"
#include "logs/structured/channels.hpp"

#include "core/fs.hpp"

#include <fstream>

namespace fs = sm::fs;
namespace logs = sm::logs;
namespace structured = sm::logs::structured;

class FileChannel final : public structured::ILogChannel {
    std::ofstream mStream;
    std::unordered_map<uint64_t, const structured::MessageInfo*> mMessages;

    void attach() override {
        auto config = structured::getMessages();
        for (const auto& message : config.messages) {
            mMessages[message.hash] = &message;
        }
    }

    ~FileChannel() override {
        mStream.close();
    }

    void postMessage(structured::LogMessagePacket packet) noexcept override {
        uint64_t hash = packet.message.hash;
        if (!mMessages.contains(hash)) {
            mStream << "Unknown message: " << hash << '\n';
            return;
        }

        auto it = mMessages.at(hash);

        mStream << fmt::vformat(it->message, *packet.args) << '\n';
    }

public:
    FileChannel(const fs::path& path) noexcept
        : mStream(path, std::ios::out)
    { }
};

structured::ILogChannel *sm::logs::structured::file(const fs::path& path) {
    return new FileChannel(path);
}
