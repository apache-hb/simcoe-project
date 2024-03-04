#pragma once

#include "render/editor/panel.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class ViewportPanel final : public IEditorPanel {
        render::Context &mContext;

        // IEditorPanel
        void draw_content() override;

    public:
        ViewportPanel(render::Context &context);
    };
}
