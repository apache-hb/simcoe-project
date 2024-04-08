#include "stdafx.hpp"

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
    eTileLightBuffer = 6, // register(u1)

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
        params[eTileLightBuffer].InitAsUnorderedAccessView(1);

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
    const DrawData &data)
{

}
