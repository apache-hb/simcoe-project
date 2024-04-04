#pragma once

#include "core/map.hpp"
#include "logs/logs.hpp"

namespace sm::ed {
    class LoggerPanel final : public logs::ILogChannel {
        struct Message {
            logs::Severity severity;
            uint32 timestamp;
            uint16 thread;

            sm::String message;
        };

        using LogMessages = sm::Vector<Message>;

        sm::Map<const logs::LogCategory*, LogMessages> mMessages;

        void draw_category(const logs::LogCategory& category) const;

        // logs::ILogChannel
        void accept(const logs::Message &message) override;

        LoggerPanel();
        ~LoggerPanel();

        static LoggerPanel gInstance;

    public:
        static LoggerPanel& get();

        bool mOpen = false;

        void draw_window();
    };
}
