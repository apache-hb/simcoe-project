#pragma once

#include "editor/draw.hpp"

#include "draw/camera.hpp"

#include "ImGuizmo.h"

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

    static constexpr const ImGuizmo::OPERATION kRotateXYZ
        = ImGuizmo::ROTATE_X
        | ImGuizmo::ROTATE_Y
        | ImGuizmo::ROTATE_Z;

    class ViewportPanel final {

        void draw_gizmo_mode(ImGuizmo::OPERATION op, ImGuizmo::OPERATION x, ImGuizmo::OPERATION y, ImGuizmo::OPERATION z) const;

        ed::EditorContext *mContext;
        flecs::entity mCamera;

        graph::Handle get_target() const {
            const ecs::CameraData *data = mCamera.get<ecs::CameraData>();
            return data->target;
        }

        ImGuizmo::OPERATION mOperation = ImGuizmo::TRANSLATE;

        ImGuizmo::OPERATION mTranslateOperation = ImGuizmo::TRANSLATE;
        ImGuizmo::OPERATION mRotateOperation = kRotateXYZ;
        ImGuizmo::OPERATION mScaleOperation = ImGuizmo::SCALE;

        ImGuizmo::MODE mMode = ImGuizmo::WORLD;

        bool mScaleViewport = true;
        ImGuiWindowFlags mFlags
            = ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoScrollWithMouse
            | ImGuiWindowFlags_NoCollapse;

        OverlayPosition mOverlayPosition = eOverlayTopLeft;

        // IEditorPanel
        void draw_content();

    public:
        ViewportPanel(ed::EditorContext *context, flecs::entity camera);

        bool mOpen = true;
        void draw_window();

        const char *getWindowTitle() const { return mCamera.name().c_str(); }

        void gizmo_settings_panel();

        static void begin_frame(draw::Camera& camera);
    };

    namespace ecs {
        // init
        void initWindowComponents(flecs::world& world);

        // update camera related systems/components
        flecs::entity addCamera(flecs::world& world, const char *name, math::float3 position, math::float3 direction);

        // query cameras and related components
        flecs::entity getPrimaryCamera(flecs::world& world);
        size_t getCameraCount(flecs::world& world);

        // editor draw functions
        void drawViewportWindows(render::IDeviceContext& ctx, flecs::world& world);
        void drawViewportMenus(flecs::world& world);
    }
}
