#include "stdafx.hpp"

#include "draw/camera.hpp"
#include "draw/common.hpp"
#include "draw/shared.hpp"

#include "draw/draw.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

using namespace sm;
using namespace draw;

static const D3D12_ROOT_SIGNATURE_FLAGS kPrimitiveRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

enum Bindings {
    eObjectBuffer = 0, // register(b0)
    eFrameBuffer = 1, // register(b1)

    ePointLightData = 2, // register(t0)
    eSpotLightData = 3, // register(t1)

    eDepthTexture = 4, // register(t2)

    eLightIndexBuffer = 5, // register(u0)

    eBindingCount
};

static void create_tiling_pipeline(render::Pipeline& pipeline, render::Context& context) {
    {
        // mvp matrix
        CD3DX12_ROOT_PARAMETER1 params[eBindingCount];
        params[eObjectBuffer].InitAsConstantBufferView(0);
        params[eFrameBuffer].InitAsConstantBufferView(1);

        params[ePointLightData].InitAsShaderResourceView(0);
        params[eSpotLightData].InitAsShaderResourceView(1);

        params[eDepthTexture].InitAsShaderResourceView(2);

        params[eLightIndexBuffer].InitAsUnorderedAccessView(0);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(1, params, 0, nullptr, kPrimitiveRootFlags);

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

    graph::PassBuilder pass = graph.compute(fmt::format("Light Culling ({})", dd.camera.name()));
    pass.read(depth, "Depth", graph::Access::eDepthRead);
    indices = pass.create(info, "Indices", graph::Access::eUnorderedAccess);

    auto& data = graph.device_data([](render::Context& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        create_tiling_pipeline(info.pipeline, context);

        return info;
    });

    pass.bind([depth, indices, dd, &data](graph::FrameGraph& graph) {
        [[maybe_unused]] auto _ = dd;

        auto& context = graph.get_context();
        auto& cmd = context.mCommandList;
        auto dsv = graph.dsv(depth);
        auto uav = graph.srv(indices);

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
