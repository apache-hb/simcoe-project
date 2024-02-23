#pragma once

#include "render/graph.hpp"

namespace sm::draw {
    class BackBuffer final : public graph::IResource {
        uint mWidth;
        uint mHeight;
        DXGI_FORMAT mFormat;

    public:
        BackBuffer(uint width, uint height, DXGI_FORMAT format)
            : mWidth(width)
            , mHeight(height)
            , mFormat(format)
        { }

        void create(graph::FrameGraph& graph, render::Context& context) override;
        void destroy(graph::FrameGraph& graph, render::Context& context) override;

        graph::Barrier transition(graph::ResourceState before, graph::ResourceState after) override;
    };

    class SceneTarget final : public graph::IResource {
        uint mWidth;
        uint mHeight;
        DXGI_FORMAT mFormat;

    public:
        SceneTarget(uint width, uint height, DXGI_FORMAT format)
            : mWidth(width)
            , mHeight(height)
            , mFormat(format)
        { }

        void create(graph::FrameGraph& graph, render::Context& context) override;
        void destroy(graph::FrameGraph& graph, render::Context& context) override;

        graph::Barrier transition(graph::ResourceState before, graph::ResourceState after) override;
    };

    class ScenePass final : public graph::IRenderPass {
    public:
        graph::ResourceId target;

        ScenePass(graph::PassBuilder& builder) {
            target = builder.produce<SceneTarget>("scene/target", D3D12_RESOURCE_STATE_RENDER_TARGET, 1920, 1080, DXGI_FORMAT_R8G8B8A8_UNORM)
                .write(D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        void create(graph::FrameGraph& graph, render::Context& context) override;
        void destroy(graph::FrameGraph& graph, render::Context& context) override;

        void build(graph::FrameGraph& graph, render::Context& context) override;
        void execute(graph::FrameGraph& graph, render::Context& context) override;
    };

    class BlitPass final : public graph::IRenderPass {
    public:
        graph::ResourceId scene;
        graph::ResourceId target;

        BlitPass(graph::PassBuilder& builder, graph::ResourceId scene, graph::ResourceId target) {
            this->scene = builder.read(scene, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            this->target = builder.write(target, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }

        void create(graph::FrameGraph& graph, render::Context& context) override;
        void destroy(graph::FrameGraph& graph, render::Context& context) override;

        void build(graph::FrameGraph& graph, render::Context& context) override;
        void execute(graph::FrameGraph& graph, render::Context& context) override;
    };

    void create_scene_classes(graph::FrameGraph& graph) {
        auto backbuffer = graph.import<BackBuffer>("swapchain backbuffer", D3D12_RESOURCE_STATE_RENDER_TARGET, 1920, 1080, DXGI_FORMAT_R8G8B8A8_UNORM);

        auto& scene = graph.pass<ScenePass>("scene");
        graph.pass<BlitPass>("blit", scene.target, backbuffer);
    }
}
