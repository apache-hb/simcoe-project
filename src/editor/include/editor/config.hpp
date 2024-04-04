#pragma once

#include "editor/panel.hpp"

namespace sm::ed {
    struct EditorContext;

    class RenderConfig final : public IEditorPanel {
        ed::EditorContext& mContext;

        // IEditorPanel
        void draw_content() override;

        void draw_adapters() const;
        void draw_allocator_info() const;
        bool draw_debug_flags() const;

    public:
        RenderConfig(EditorContext& context)
            : IEditorPanel("Render Config")
            , mContext(context)
        { }
    };
}
