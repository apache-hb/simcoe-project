#pragma once

#include "render/editor/panel.hpp"
#include "render/editor/scene.hpp"

namespace sm::editor {
    class InspectorPanel final : public IEditorPanel {
        render::Context &mContext;
        ViewportPanel &mViewport;

        // IEditorPanel
        void draw_content() override;

        void draw_mesh(ItemIndex index);
        void draw_node(ItemIndex index);
        void draw_camera(ItemIndex index);
        void draw_image(ItemIndex index);
        void draw_material(ItemIndex index);

    public:
        InspectorPanel(render::Context &context, ViewportPanel &viewport);
    };
}
