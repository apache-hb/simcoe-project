#pragma once

#include "draw/camera.hpp"

#include "render/render.hpp"

namespace sm::ed {
    struct CameraData {
        draw::Camera camera;
        graph::Handle target;
    };

    struct EditorContext final : public render::Context {
        using Super = render::Context;
        using Super::Super;

        EditorContext(const render::RenderConfig& config);

        void on_create() override;
        void on_destroy() override;

        void setup_framegraph(graph::FrameGraph& graph) override;

        void tick(float dt);

        void render();

        CameraData& add_camera();

        draw::Camera& get_active_camera() { return mActiveCamera->camera; }

        sm::Span<sm::UniquePtr<CameraData>> get_cameras() { return mCameras; }

        void erase_camera(size_t index);

        std::optional<world::AnyIndex> selected;
        render::SrvIndex index;
        input::InputService input;

        // should the device be recreated?
        bool recreate_device = false;

    private:
        sm::Vector<sm::UniquePtr<CameraData>> mCameras;
        CameraData *mActiveCamera = nullptr;

        CameraData& push_camera(math::uint2 size);
        void imgui(graph::FrameGraph& graph, graph::Handle render_target);
    };
}
