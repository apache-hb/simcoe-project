#pragma once

#include "core/map.hpp"
#include "logs/logs.hpp"

#include "editor/panel.hpp"

namespace sm::ed {
    class LoggerPanel final : public logs::ILogChannel, public IEditorPanel {
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

        // IEditorPanel
        void draw_content() override;

        LoggerPanel();
        ~LoggerPanel();

        static LoggerPanel gInstance;

    public:
        static LoggerPanel& get();
    };
}
