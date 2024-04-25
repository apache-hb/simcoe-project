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

        void tick(float dt);

        void render();

        // CameraData& add_camera();

        // draw::Camera& get_active_camera() { return mActiveCamera->camera; }

        // sm::Span<sm::UniquePtr<CameraData>> get_cameras() { return mCameras; }

        void erase_camera(size_t index);

        std::optional<world::AnyIndex> selected;
        render::SrvIndex index;
        input::InputService input;

        // should the device be recreated?
        bool recreate_device = false;

        flecs::world& ecs() { return mSystem; }

        flecs::entity getCamera() { return mCamera; }

    private:
        // sm::Vector<sm::UniquePtr<CameraData>> mCameras;
        // CameraData *mActiveCamera = nullptr;

        flecs::world mSystem;
        flecs::entity mCamera;

        void on_setup() override;

        // CameraData& push_camera(math::uint2 size);
        void imgui(graph::FrameGraph& graph, graph::Handle render_target);
    };
}
