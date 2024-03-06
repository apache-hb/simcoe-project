#pragma once

#include "render/editor/panel.hpp"
#include "render/editor/scene.hpp"

namespace sm::editor {
    class InspectorPanel final : public IEditorPanel {
        render::Context &mContext;
        ItemIndex mSelected = {ItemIndex::eNone, 0};

        // IEditorPanel
        void draw_content() override;

    public:
        InspectorPanel(render::Context &context);

        void set_index(ItemIndex index) { mSelected = index; }
    };
}
