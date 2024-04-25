#pragma once

// #include "draw/camera.hpp"

#include "input/input.hpp"

#include "render/render.hpp"

#include <flecs.h>

namespace sm::ed {
    namespace ecs {
        struct CameraData {
            graph::Handle target;
            graph::Handle depth;
        };
    }

    struct EditorContext final : public render::IDeviceContext {
        using Super = render::IDeviceContext;
        using Super::Super;

        EditorContext(const render::RenderConfig& config);

        void init();

        void on_create() override;
        void on_destroy() override;

        void setup_framegraph(graph::FrameGraph& graph) override;

        void render();

        // CameraData& add_camera();

        // draw::Camera& get_active_camera() { return mActiveCamera->camera; }

        // sm::Span<sm::UniquePtr<CameraData>> get_cameras() { return mCameras; }

        void erase_camera(size_t index);

        std::optional<world::AnyIndex> selected;
        render::SrvIndex index;
        input::InputService input;

        void markDeviceDirty() { mDeviceDirty = true; }

        flecs::world& getWorld() { return mSystem; }
        flecs::entity getCamera() { return mCamera; }

    private:
        // should the device be recreated?
        bool mDeviceDirty = false;

        // do we need to rebuild the framegraph?
        bool mFrameGraphDirty = false;

        flecs::world mSystem;
        flecs::entity mCamera;
        flecs::entity mCameraObserver;

        void on_setup() override;

        // CameraData& push_camera(math::uint2 size);
        void imgui(graph::FrameGraph& graph, graph::Handle render_target);
    };
}
