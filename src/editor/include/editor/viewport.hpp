#pragma once

#include "editor/draw.hpp"
#include "editor/panel.hpp"
#include "editor/scene.hpp"

#include "draw/camera.hpp"

#include "ImGuizmo.h"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class ViewportPanel final : public IEditorPanel {
        static constexpr const ImGuizmo::OPERATION kRotateXYZ
            = ImGuizmo::ROTATE_X
            | ImGuizmo::ROTATE_Y
            | ImGuizmo::ROTATE_Z;

        void draw_gizmo_mode(ImGuizmo::OPERATION op, ImGuizmo::OPERATION x, ImGuizmo::OPERATION y, ImGuizmo::OPERATION z) const;
        void draw_rotate_mode() const;

        ed::EditorContext &mContext;
        size_t mCameraIndex;

        auto& get_camera() { return mContext.cameras[mCameraIndex]; }

        ItemIndex mSelected = ItemIndex::none();
        ImGuizmo::OPERATION mOperation = ImGuizmo::TRANSLATE;

        ImGuizmo::OPERATION mTranslateOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::OPERATION mRotateOperation = kRotateXYZ;
        ImGuizmo::OPERATION mScaleOperation = ImGuizmo::SCALE;

        ImGuizmo::MODE mMode = ImGuizmo::WORLD;

        bool mScaleViewport = true;

        // IEditorPanel
        void draw_content() override;

    public:
        ViewportPanel(ed::EditorContext &context, size_t index);

        bool draw_window() override;

        void select(ItemIndex index) { mSelected = index; }
        ItemIndex get_selected() const { return mSelected; }

        void gizmo_settings_panel();

        static void begin_frame(draw::Camera& camera);
    };
}
