#pragma once

#include "core/map.hpp"
#include "logs/logs.hpp"

#include "logs/structured/logger.hpp"

namespace sm::ed {
    namespace structured = logs::structured;

    class LoggerPanel final : public logs::structured::ILogChannel {

        struct Message {
            logs::Severity severity;
            uint32 timestamp;
            uint16 thread;

            sm::String message;
        };

        using LogMessages = std::vector<Message>;

        sm::Map<const structured::CategoryInfo*, LogMessages> mMessages;

        void drawLogCategory(const structured::CategoryInfo& category) const;

        // structured::ILogChannel
        void attach() override { }
        void postMessage(structured::MessagePacket packet) noexcept override;

        LoggerPanel();
        ~LoggerPanel();

        static LoggerPanel gInstance;

    public:
        static LoggerPanel& get();

        bool mOpen = false;

        void draw_window();
    };
}
