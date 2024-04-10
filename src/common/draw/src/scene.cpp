#include "stdafx.hpp"

#include "draw/draw.hpp"
#include "draw/camera.hpp"

#include "render/render.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

using namespace sm;
using namespace sm::math;
using namespace sm::world;

static void draw_node(render::Context& context, ID3D12GraphicsCommandList1 *commands, const draw::Camera& camera, IndexOf<world::Node> index, const float4x4& parent) {
    float ar = camera.config().aspect_ratio();
    const auto& node = context.mWorld.nodes[index];

    auto model = (parent * node.transform.matrix());
    float4x4 mvp = camera.mvp(ar, model.transpose()).transpose();

    commands->SetGraphicsRoot32BitConstants(0, 16, mvp.data(), 0);

    for (IndexOf i : node.models) {
        const auto& object = context.mMeshes[i];
        commands->IASetVertexBuffers(0, 1, &object.vbo_view);
        commands->IASetIndexBuffer(&object.ibo_view);

        commands->DrawIndexedInstanced(object.index_count, 1, 0, 0, 0);
    }

    for (IndexOf i : node.children) {
        draw_node(context, commands, camera, i, model);
    }
}

static const D3D12_ROOT_SIGNATURE_FLAGS kPrimitiveRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

static void create_primitive_pipeline(render::Pipeline& pipeline, const draw::ViewportConfig& config, render::Context& context) {
    {
        // mvp matrix
        CD3DX12_ROOT_PARAMETER1 params[1];
        params[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(1, params, 0, nullptr, kPrimitiveRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto ps = context.mConfig.bundle.get_shader_bytecode("primitive.ps");
        auto vs = context.mConfig.bundle.get_shader_bytecode("primitive.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(world::Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(world::Vertex, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kDesc = {
            .pRootSignature = pipeline.signature.get(),
            .VS = CD3DX12_SHADER_BYTECODE(vs.data(), vs.size()),
            .PS = CD3DX12_SHADER_BYTECODE(ps.data(), ps.size()),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
            .InputLayout = { kInputElements, _countof(kInputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { config.colour },
            .DSVFormat = config.depth,
            .SampleDesc = { 1, 0 },
        };

        auto& device = context.mDevice;

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void draw::opaque(graph::FrameGraph& graph, graph::Handle& target, const Camera& camera) {
    auto config = camera.config();
    graph::ResourceInfo depth_info = {
        .sz = graph::ResourceSize::tex2d(config.size),
        .format = config.depth,
        .clear = graph::clear_depth(1.f)
    };

    graph::ResourceInfo target_info = {
        .sz = graph::ResourceSize::tex2d(config.size),
        .format = config.colour,
        .clear = graph::clear_colour(render::kClearColour)
    };

    graph::PassBuilder pass = graph.graphics(fmt::format("Opaque ({})", camera.name()));
    target = pass.create(target_info, "Target", graph::Usage::eRenderTarget);
    auto depth = pass.create(depth_info, "Depth", graph::Usage::eDepthWrite);

    auto& data = graph.device_data([config](render::Context& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        create_primitive_pipeline(info.pipeline, config, context);

        return info;
    });

    pass.bind([target=target, depth=depth, &data, &camera](graph::RenderContext& ctx) {
        auto& context = ctx.context;
        auto& graph = ctx.graph;
        auto *cmd = ctx.commands;
        auto viewport = camera.viewport();
        auto rtv = graph.rtv(target);
        auto dsv = graph.dsv(depth);

        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);
        auto dsv_cpu = context.mDsvPool.cpu_handle(dsv);

        cmd->SetGraphicsRootSignature(*data.pipeline.signature);
        cmd->SetPipelineState(*data.pipeline.pso);

        cmd->RSSetViewports(1, &viewport.mViewport);
        cmd->RSSetScissorRects(1, &viewport.mScissorRect);

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, &dsv_cpu);
        cmd->ClearRenderTargetView(rtv_cpu, render::kClearColour.data(), 0, nullptr);
        cmd->ClearDepthStencilView(dsv_cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const auto& scene = context.get_scene();
        draw_node(context, cmd, camera, scene.root, math::float4x4::identity());
    });
}
