#pragma once

#include "render/editor/panel.hpp"

#include "core/map.hpp"
#include "core/vector.hpp"

#include "ImNodesEz.h"

namespace sm::render {
    struct Context;
}

namespace sm::editor {
    class GraphPanel final : public IEditorPanel {
        render::Context& mContext;

        void draw_graph();
        void draw_lifetimes();

        // IEditorPanel
        void draw_content() override;

    public:
        GraphPanel(render::Context& context);
    };
}
