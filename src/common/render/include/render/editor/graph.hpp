#pragma once

#include "render/editor/panel.hpp"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class GraphPanel final : public IEditorPanel {
        render::Context& mContext;

        // IEditorPanel
        void draw_content() override;

    public:
        GraphPanel(render::Context& context);
    };
}
