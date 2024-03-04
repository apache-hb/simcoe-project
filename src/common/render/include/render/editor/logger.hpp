#pragma once

#include "logs/logs.hpp"
#include "core/array.hpp"

#include "render/editor/panel.hpp"

namespace sm::editor {
    class LoggerPanel final : public logs::ILogChannel, public IEditorPanel {
        struct Message {
            logs::Severity severity;
            uint32 timestamp;
            uint16 thread;

            sm::String message;
        };

        using LogMessages = sm::Vector<Message>;

        static constexpr size_t kCategoryCount = (size_t)logs::Category::eCount;

        sm::Array<LogMessages, kCategoryCount> mMessages;

        void draw_category(logs::Category category) const;

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
