#pragma once

#include "render/render.hpp"

namespace sm::render {
    class BeginCommands : public render::IRenderNode {

        void build(render::Context& ctx) override {
            ctx.begin_frame();

            auto& commands = ctx.get_direct_commands();
            auto& heap = ctx.get_srv_heap();

            ID3D12DescriptorHeap *heaps[] = { heap.get_heap() };
            commands->SetDescriptorHeaps(std::size(heaps), heaps);
        }
    };

    class EndCommands : public render::IRenderNode {

        void build(render::Context& ctx) override {
            ctx.end_frame();
        }
    };

    class WorldCommands : public render::IRenderNode {
        static constexpr inline const char *kVertexShader = "shaders/object.vs.cso";
        static constexpr inline const char *kPixelShader = "shaders/object.ps.cso";

        static constexpr inline const char *kTextureSlot = "texture";
        static constexpr inline const char *kCameraSlot = "camera";

        bundle::AssetBundle& m_assets;

        // viewport/scissor
        D3D12_VIEWPORT m_viewport;
        D3D12_RECT m_scissor;
        math::uint2 m_size;

        void update_viewport();

        // pipeline
        rhi::PipelineState m_pipeline;

        void setup_pipeline(render::Context& ctx);

        rhi::ResourceObject m_background;
        render::SrvHeapIndex m_background_index = render::SrvHeapIndex::eInvalid;

        void setup_background(render::Context& context);

        // gpu resources
        rhi::ResourceObject m_vbo;
        D3D12_VERTEX_BUFFER_VIEW m_vbo_view{};

        rhi::ResourceObject m_ibo;
        D3D12_INDEX_BUFFER_VIEW m_ibo_view{};

        SrvHeapIndex m_texture_index = SrvHeapIndex::eInvalid;
        rhi::ResourceObject m_texture;

        void setup_assets(render::Context& ctx);

        SrvHeapIndex m_camera_index = SrvHeapIndex::eInvalid;
        CameraBuffer m_camera{};
        CameraBuffer *m_camera_data = nullptr;
        rhi::ResourceObject m_camera_resource;

        void setup_camera(render::Context& ctx);
        void update_camera();

        void create(render::Context& ctx) override {
            m_size = ctx.get_rhi().get_swapchain_size();
            update_viewport();

            setup_pipeline(ctx);
            setup_assets(ctx);
            setup_camera(ctx);
            setup_background(ctx);
            update_camera();
        }

        void resize(render::Context& ctx, math::uint2 size) override {
            m_size = size;
            update_camera();
            update_viewport();
        }

        void build(render::Context& context) override {
            auto& rhi = context.get_rhi();
            auto& heap = context.get_srv_heap();
            auto& direct = rhi.get_direct_commands();

            direct.set_root_signature(m_pipeline);
            direct.set_pipeline_state(m_pipeline);

            // use 2 tables in case the indices are not contiguous
            direct->SetGraphicsRootDescriptorTable(0, heap.gpu_handle(m_background_index));
            direct->SetGraphicsRootDescriptorTable(1, heap.gpu_handle(m_camera_index));
            direct->RSSetViewports(1, &m_viewport);
            direct->RSSetScissorRects(1, &m_scissor);

            direct->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            direct->IASetVertexBuffers(0, 1, &m_vbo_view);
            direct->IASetIndexBuffer(&m_ibo_view);
            direct->DrawIndexedInstanced(6, 1, 0, 0, 0);
        }

    public:
        WorldCommands(bundle::AssetBundle& assets)
            : m_assets(assets)
        { }
    };

    class PresentCommands : public render::IRenderNode {
        void build(render::Context& ctx) override {
            ctx.present();
        }
    };
}
