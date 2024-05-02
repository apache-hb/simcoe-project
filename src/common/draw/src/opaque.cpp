#include "stdafx.hpp"

#include "draw/draw.hpp"
#include "draw/camera.hpp"

#include "render/render.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

using namespace sm;
using namespace sm::math;
using namespace sm::world;

static uint tryGetAlbedoIndex(render::IDeviceContext& self, IndexOf<world::Material> index) {
    if (index == world::kInvalidIndex) {
        return UINT_MAX;
    }

    const auto& material = self.mWorld.get(index);
    if (material.albedo_texture.image == world::kInvalidIndex) {
        return UINT_MAX;
    }

    const render::TextureResource& texture = self.mImages.at(material.albedo_texture.image);
    return static_cast<uint>(texture.srv);
}

static void draw_node(render::IDeviceContext& context, ID3D12GraphicsCommandList1 *commands, const draw::Camera& camera, IndexOf<world::Node> index, const float4x4& parent) {
    float ar = camera.config().getAspectRatio();
    const auto& node = context.mWorld.get(index);

    auto model = (parent * node.transform.matrix());
    float4x4 mvp = camera.mvp(ar, model.transpose()).transpose();

    commands->SetGraphicsRoot32BitConstants(0, 16, mvp.data(), 0);

    for (IndexOf i : node.models) {
        const auto& object = context.mMeshes[i];
        commands->IASetVertexBuffers(0, 1, &object.vbo.view);
        commands->IASetIndexBuffer(&object.ibo.view);

        const auto& info = context.mWorld.get(i);
        uint albedoIndex = tryGetAlbedoIndex(context, info.material);
        commands->SetGraphicsRoot32BitConstant(0, albedoIndex, 16);

        commands->DrawIndexedInstanced(object.ibo.length, 1, 0, 0, 0);
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
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
    ;

struct ObjectData {
    float4x4 mvp;
    uint albedoTextureIndex;
};

enum {
    eCameraBuffer, // register(b0)

    eTextureArray, // register(t0)

    eBindingCount
};

static void create_primitive_pipeline(render::Pipeline& pipeline, const draw::ViewportConfig& config, render::IDeviceContext& context) {
    {
        // object data
        // mvp + albedo texture index
        CD3DX12_ROOT_PARAMETER1 params[eBindingCount];
        params[eCameraBuffer].InitAsConstants(16 + 1, 0);

        // all textures
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
        params[eTextureArray].InitAsDescriptorTable(_countof(ranges), ranges, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC samplers[1];
        samplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(_countof(params), params, _countof(samplers), samplers, kPrimitiveRootFlags);

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

        auto device = context.getDevice();

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void draw::opaque(graph::FrameGraph& graph, graph::Handle& target, graph::Handle& depth, const Camera& camera) {
    auto config = camera.config();
    graph::ResourceInfo depth_info = graph::ResourceInfo::tex2d(config.size, config.getDepthFormat())
        .clearDepthStencil(1.f, 0);

    graph::ResourceInfo target_info = graph::ResourceInfo::tex2d(config.size, config.getColourFormat())
        .clearColour(render::kClearColour);

    graph::PassBuilder pass = graph.graphics(fmt::format("Opaque ({})", camera.name()));
    target = pass.create(target_info, "Target", graph::Usage::eRenderTarget);
    depth = pass.create(depth_info, "Depth", graph::Usage::eDepthWrite);

    auto& data = graph.newDeviceData([config](render::IDeviceContext& context) {
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

        cmd->SetGraphicsRootDescriptorTable(eTextureArray, context.mSrvPool.get()->GetGPUDescriptorHandleForHeapStart());

        const auto& scene = context.get_scene();
        draw_node(context, cmd, camera, scene.root, math::float4x4::identity());
    });
}
