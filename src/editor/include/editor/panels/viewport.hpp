#pragma once

#include "editor/draw.hpp"
#include "editor/panels/scene.hpp"

#include "draw/camera.hpp"

#include "ImGuizmo.h"

namespace sm::render {
    struct Context;
}

namespace sm::ed {
    class ViewportPanel final {
        static constexpr const ImGuizmo::OPERATION kRotateXYZ
            = ImGuizmo::ROTATE_X
            | ImGuizmo::ROTATE_Y
            | ImGuizmo::ROTATE_Z;

        void draw_gizmo_mode(ImGuizmo::OPERATION op, ImGuizmo::OPERATION x, ImGuizmo::OPERATION y, ImGuizmo::OPERATION z) const;
        void draw_rotate_mode() const;

        ed::EditorContext &mContext;
        size_t mCameraIndex;

        auto& get_camera() { return mContext.cameras[mCameraIndex]; }

        ImGuizmo::OPERATION mOperation = ImGuizmo::TRANSLATE;

        ImGuizmo::OPERATION mTranslateOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::OPERATION mRotateOperation = kRotateXYZ;
        ImGuizmo::OPERATION mScaleOperation = ImGuizmo::SCALE;

        ImGuizmo::MODE mMode = ImGuizmo::WORLD;

        bool mScaleViewport = true;
        sm::String mName;
        ImGuiWindowFlags mFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        // IEditorPanel
        void draw_content();

    public:
        ViewportPanel(ed::EditorContext &context, size_t index);

        bool mOpen = true;
        void draw_window();

        void gizmo_settings_panel();
        const char *get_title() const { return mName.c_str(); }

        static void begin_frame(draw::Camera& camera);
    };
}
