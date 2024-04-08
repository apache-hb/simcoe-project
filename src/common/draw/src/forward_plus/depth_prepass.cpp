#include "draw/camera.hpp"
#include "stdafx.hpp"

#include "draw/draw.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

using namespace sm;
using namespace sm::draw;
using namespace sm::world;

// todo: dedup this with opaque
static void draw_node(render::Context& context, const draw::Camera& camera, IndexOf<Node> index, const float4x4& parent) {
    float ar = camera.config().aspect_ratio();
    const auto& node = context.mWorld.nodes[index];

    auto model = (parent * node.transform.matrix());
    float4x4 mvp = camera.mvp(ar, model.transpose()).transpose();

    auto& cmd = context.mCommandList;
    cmd->SetGraphicsRoot32BitConstants(0, 16, mvp.data(), 0);

    for (IndexOf i : node.models) {
        const auto& object = context.mMeshes[i];
        cmd->IASetVertexBuffers(0, 1, &object.vbo_view);
        cmd->IASetIndexBuffer(&object.ibo_view);

        cmd->DrawIndexedInstanced(object.index_count, 1, 0, 0, 0);
    }

    for (IndexOf i : node.children) {
        draw_node(context, camera, i, model);
    }
}

// TODO: should this be configurable
static constexpr DXGI_FORMAT kDepthFormat = DXGI_FORMAT_D32_FLOAT;

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kPrePassRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

static void create_pipeline(render::Pipeline& pipeline, render::Context& context) {
    {
        CD3DX12_ROOT_PARAMETER1 params[1];
        params[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(_countof(params), params, 0, nullptr, kPrePassRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        // TODO: need to get the alpha test pass as well
        auto vs = context.mConfig.bundle.get_shader_bytecode("forward_plus_depth_pass.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(world::Vertex, position) },

            // uncomment this when we use the alpha test version
            // { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(world::Vertex, texcoord) },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kPipeline = {
            .pRootSignature = pipeline.signature.get(),
            .VS = CD3DX12_SHADER_BYTECODE(vs.data(), vs.size()),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT), // TODO: is this blend state correct
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .InputLayout = { kInputElements, _countof(kInputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 0,
            .DSVFormat = kDepthFormat,
            .SampleDesc = { 1, 0 },
        };
        auto& device = context.mDevice;

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kPipeline, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void forward_plus::depth_prepass(
    graph::FrameGraph &graph,
    graph::Handle &depth_target,
    const DrawData &drawdata)
{
    const auto& config = drawdata.camera.config();

    graph::ResourceInfo info = {
        .size = config.size,
        .format = kDepthFormat,
        .clear = graph::clear_depth(1.f),
    };

    graph::PassBuilder pass = graph.pass(fmt::format("Depth Prepass ({})", drawdata.camera.name()));
    depth_target = pass.create(info, "Depth", graph::Access::eDepthTarget);

    // pass.side_effects(true);

    auto& data = graph.device_data([](render::Context& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        create_pipeline(info.pipeline, context);

        return info;
    });

    pass.bind([depth_target, &data, dd=drawdata](graph::FrameGraph& graph) {
        auto& context = graph.get_context();
        auto& cmd = context.mCommandList;
        auto viewport = dd.camera.viewport();

        auto dsv = graph.dsv(depth_target);
        auto dsv_cpu = context.mDsvPool.cpu_handle(dsv);

        cmd->SetGraphicsRootSignature(data.pipeline.signature.get());
        cmd->SetPipelineState(data.pipeline.pso.get());

        cmd->RSSetViewports(1, &viewport.mViewport);
        cmd->RSSetScissorRects(1, &viewport.mScissorRect);

        cmd->OMSetRenderTargets(0, nullptr, false, &dsv_cpu);
        cmd->ClearDepthStencilView(dsv_cpu, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        const auto& scene = context.get_scene();
        draw_node(context, dd.camera, scene.root, float4x4::identity());
    });
}