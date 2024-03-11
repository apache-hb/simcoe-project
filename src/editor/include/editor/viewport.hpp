#pragma once

#include "editor/panel.hpp"
#include "editor/scene.hpp"

#include "render/camera.hpp"

#include "ImGuizmo.h"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class ViewportPanel final : public IEditorPanel {
        render::Context &mContext;

        ItemIndex mSelected = ItemIndex::none();
        ImGuizmo::OPERATION mOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::MODE mMode = ImGuizmo::WORLD;

        bool mScaleViewport = true;

        // IEditorPanel
        void draw_content() override;

    public:
        ViewportPanel(render::Context &context);

        void select(ItemIndex index) { mSelected = index; }
        ItemIndex get_selected() const { return mSelected; }

        void gizmo_settings_panel();

        static void begin_frame(draw::Camera& camera);
    };
}
