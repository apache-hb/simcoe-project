#pragma once

#include "core/map.hpp"
#include "logs/logs.hpp"

#include "logs/logger.hpp"

namespace sm::ed {

    class LoggerPanel final : public logs::ILogChannel {

        struct Message {
            logs::Severity severity;
            uint32 timestamp;
            uint16 thread;

            sm::String message;
        };

        using LogMessages = std::vector<Message>;

        sm::Map<const logs::CategoryInfo*, LogMessages> mMessages;

        void drawLogCategory(const logs::CategoryInfo& category) const;

        // structured::ILogChannel
        void attach() override { }
        void postMessage(logs::MessagePacket packet) noexcept override;

        LoggerPanel();
        ~LoggerPanel();

        static LoggerPanel gInstance;

    public:
        static LoggerPanel& get();

        bool mOpen = false;

        void draw_window();
    };
}
