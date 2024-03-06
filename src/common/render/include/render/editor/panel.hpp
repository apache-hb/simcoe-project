#pragma once

#include "core/text.hpp"

#include "imgui/imgui.h"

namespace sm::editor {
    class IEditorPanel {
        bool mOpen = true;
        sm::String mTitle;

        virtual void draw_content() = 0;

    protected:
        ImGuiWindowFlags mFlags;

        IEditorPanel(sm::StringView title, ImGuiWindowFlags flags = ImGuiWindowFlags_None)
            : mTitle(title)
            , mFlags(flags)
        { }

    public:
        virtual ~IEditorPanel() = default;

        void draw_menu_item(const char *shortcut = nullptr);
        void draw_window();
        void draw_section();
    };
}
