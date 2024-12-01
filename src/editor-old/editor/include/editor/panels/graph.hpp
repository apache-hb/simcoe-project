#pragma once

#include "editor/panels/panel.hpp"

#include "core/map.hpp"
#include "core/adt/vector.hpp"

#include "ImNodesEz.h"

namespace sm::render {
    struct IDeviceContext;
}

namespace sm::ed {
    class GraphPanel final : public IEditorPanel {
        render::IDeviceContext& mContext;

        void draw_graph();
        void draw_lifetimes();

        // IEditorPanel
        void draw_content() override;

    public:
        GraphPanel(render::IDeviceContext& context);
    };
}
