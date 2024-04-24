#pragma once

#include "editor/draw.hpp"

#include "draw/camera.hpp"

#include "ImGuizmo.h"

namespace sm::render {
    struct IDeviceContext;
}

namespace sm::ed {
    enum OverlayPosition {
        eOverlayClosed = -1,

        eOverlayTop = (1 << 0),
        eOverlayLeft = (1 << 1),

        eOverlayTopLeft = eOverlayTop | eOverlayLeft,
        eOverlayTopRight = eOverlayTop,
        eOverlayBottomLeft = eOverlayLeft,
        eOverlayBottomRight = 0,
    };

    class ViewportPanel final {
        static constexpr const ImGuizmo::OPERATION kRotateXYZ
            = ImGuizmo::ROTATE_X
            | ImGuizmo::ROTATE_Y
            | ImGuizmo::ROTATE_Z;


        void draw_gizmo_mode(ImGuizmo::OPERATION op, ImGuizmo::OPERATION x, ImGuizmo::OPERATION y, ImGuizmo::OPERATION z) const;
        void draw_rotate_mode() const;

        ed::EditorContext *mContext;
        ed::CameraData *mCamera;

        draw::Camera& get_camera() { return mCamera->camera; }
        graph::Handle& get_target() { return mCamera->target; }

        ImGuizmo::OPERATION mOperation = ImGuizmo::TRANSLATE;

        ImGuizmo::OPERATION mTranslateOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::OPERATION mRotateOperation = kRotateXYZ;
        ImGuizmo::OPERATION mScaleOperation = ImGuizmo::SCALE;

        ImGuizmo::MODE mMode = ImGuizmo::WORLD;

        bool mScaleViewport = true;
        sm::String mName;
        ImGuiWindowFlags mFlags
            = ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoCollapse;

        OverlayPosition mOverlayPosition = eOverlayTopLeft;

        // IEditorPanel
        void draw_content();

    public:
        ViewportPanel(ed::EditorContext *context, ed::CameraData *camera);

        bool mOpen = true;
        void draw_window();

        void gizmo_settings_panel();
        const char *get_title() const { return mName.c_str(); }

        static void begin_frame(draw::Camera& camera);
    };
}
