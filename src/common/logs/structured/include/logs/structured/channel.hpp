#pragma once

#include "logs/structured/logging.hpp"

#include "core/fs.hpp"

namespace sm::db {
    class Connection;
}

namespace sm::logs::structured {
    struct LogMessagePacket {
        const MessageInfo& message;
        uint64_t timestamp;
        std::shared_ptr<ArgStore> args;
    };

    class ILogChannel {
    public:
        virtual ~ILogChannel() = default;

        virtual void attach() = 0;

        virtual void postMessage(LogMessagePacket packet) noexcept = 0;
    };

    class Logger {
        std::vector<std::unique_ptr<ILogChannel>> mChannels;

    public:
        void addChannel(std::unique_ptr<ILogChannel> channel);

        void cleanup() noexcept;

        void postMessage(const MessageInfo& message, ArgStore args) noexcept;

        static Logger& instance() noexcept;
    };

    ILogChannel *console();
    ILogChannel *file(const fs::path& path);
    ILogChannel *database(db::Connection& connection);
}
