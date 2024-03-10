#pragma once

#include "render/editor/panel.hpp"

#include "ImNodesEz.h"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class GraphPanel final : public IEditorPanel {
        render::Context& mContext;

        ImNodes::CanvasState mCanvasState{};

        void draw_graph();
        void draw_lifetimes();

        // IEditorPanel
        void draw_content() override;

    public:
        GraphPanel(render::Context& context);
    };
}
