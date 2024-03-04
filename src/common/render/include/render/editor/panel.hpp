#pragma once

#include "core/text.hpp"

namespace sm::editor {
    class IEditorPanel {
        bool mOpen = true;
        sm::String mTitle;

        virtual void draw_content() = 0;

    protected:
        IEditorPanel(sm::StringView title)
            : mTitle(title)
        { }

    public:
        virtual ~IEditorPanel() = default;

        void draw_menu_item(const char *shortcut = nullptr);
        void draw_window();
        void draw_section();
    };
}
