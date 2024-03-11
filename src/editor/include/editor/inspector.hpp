#pragma once

#include "editor/panel.hpp"
#include "editor/scene.hpp"

namespace sm::ed {
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
