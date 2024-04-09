#include "stdafx.hpp"

#include "draw/camera.hpp"
#include "draw/common.hpp"
#include "draw/shared.hpp"

#include "draw/draw.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

using namespace sm;
using namespace draw;

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kComputeRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

enum Bindings {
    eFrameBuffer, // register(b1)

    ePointLightData, // register(t0)
    eSpotLightData, // register(t1)

    eDepthTexture, // register(t2)

    eLightIndexBuffer, // register(u0)

    eBindingCount
};

static void create_tiling_pipeline(render::Pipeline& pipeline, render::Context& context) {
    {
        CD3DX12_ROOT_PARAMETER1 params[eBindingCount];
        params[eFrameBuffer].InitAsConstantBufferView(0);

        params[ePointLightData].InitAsShaderResourceView(0);
        params[eSpotLightData].InitAsShaderResourceView(1);

        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
        params[eDepthTexture].InitAsDescriptorTable(_countof(ranges), ranges);

        params[eLightIndexBuffer].InitAsUnorderedAccessView(0);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(_countof(params), params, 0, nullptr, kComputeRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto cs = context.mConfig.bundle.get_shader_bytecode("forward_plus_tiling.cs");

        const D3D12_COMPUTE_PIPELINE_STATE_DESC kDesc = {
            .pRootSignature = pipeline.signature.get(),
            .CS = CD3DX12_SHADER_BYTECODE(cs.data(), cs.size()),
            .NodeMask = 0,
            .Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
        };

        auto& device = context.mDevice;

        SM_ASSERT_HR(device->CreateComputePipelineState(&kDesc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void forward_plus::light_culling(
    graph::FrameGraph &graph,
    graph::Handle &indices,
    graph::Handle depth,
    DrawData dd)
{
    const auto& camera = dd.camera.config();
    uint tile_count = draw::get_tile_count(camera.size, 16);

    graph::ResourceInfo info = {
        .sz = graph::ResourceSize::buffer(sizeof(TileLightData) * tile_count),
        .format = DXGI_FORMAT_UNKNOWN,
    };

    graph::PassBuilder pass = graph.compute(fmt::format("Forward+ Light Culling ({})", dd.camera.name()));
    pass.read(depth, "Depth", graph::Access::eDepthRead);
    indices = pass.create(info, "Indices", graph::Access::eUnorderedAccess);

    auto& data = graph.device_data([](render::Context& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        create_tiling_pipeline(info.pipeline, context);

        return info;
    });

    pass.bind([depth, indices, dd, &data](graph::RenderContext& ctx) {
        [[maybe_unused]] auto _ = dd;

        auto& context = ctx.context;
        auto *cmd = ctx.commands;
        auto& graph = ctx.graph;

        auto dsv = graph.dsv(depth);
        auto uav = graph.uav(indices);

        auto dsv_gpu = context.mDsvPool.gpu_handle(dsv);
        auto uav_gpu = context.mSrvPool.gpu_handle(uav);

        cmd->SetComputeRootSignature(data.pipeline.signature.get());
        cmd->SetPipelineState(data.pipeline.pso.get());

        // TODO: everything else

        cmd->SetComputeRootDescriptorTable(eDepthTexture, dsv_gpu);

        cmd->SetComputeRootDescriptorTable(ePointLightData, uav_gpu);

        cmd->Dispatch(TILE_SIZE, TILE_SIZE, 1);
    });
}
