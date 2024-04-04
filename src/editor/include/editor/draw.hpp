#pragma once

#include "draw/camera.hpp"
#include "render/render.hpp"

namespace sm::ed {
    struct EditorContext final : public render::Context {
        using Super = render::Context;
        using Super::Super;

        EditorContext(const render::RenderConfig& config);

        void on_create() override;
        void on_destroy() override;

        void setup_framegraph(graph::FrameGraph& graph) override;

        void tick(float dt);

        render::SrvIndex index;

        struct CameraData {
            draw::Camera camera;
            graph::Handle target;
        };

        sm::Vector<CameraData> cameras;
        size_t active = 0;

    private:
        void imgui(graph::FrameGraph& graph, graph::Handle target);
    };
}
