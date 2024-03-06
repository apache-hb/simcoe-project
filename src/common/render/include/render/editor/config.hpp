#pragma once

#include "render/editor/panel.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class RenderConfig final : public IEditorPanel {
        render::Context& mContext;

        // IEditorPanel
        void draw_content() override;

        void draw_adapters() const;
        void draw_allocator_info() const;
        bool draw_debug_flags() const;

    public:
        RenderConfig(render::Context& context)
            : IEditorPanel("Render Config")
            , mContext(context)
        { }
    };
}
