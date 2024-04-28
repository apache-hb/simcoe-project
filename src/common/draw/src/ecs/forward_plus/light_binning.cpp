#include "stdafx.hpp"

#include "draw/draw.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

using namespace sm;
using namespace sm::draw;

enum {
    eViewportBuffer, // register(b0)

    ePointLightData, // register(t0)
    eSpotLightData, // register(t1)

    eDepthTexture, // register(t2)

    eLightIndexBuffer, // register(u0)

    eBindingCount
};

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kBinningPassRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;

static void createLightBinningPipeline(
    render::Pipeline& pipeline,
    render::IDeviceContext& context
)
{
    {
        CD3DX12_ROOT_PARAMETER1 params[eBindingCount];
        params[eViewportBuffer].InitAsConstantBufferView(0);

        params[ePointLightData].InitAsShaderResourceView(0);
        params[eSpotLightData].InitAsShaderResourceView(1);

        CD3DX12_DESCRIPTOR_RANGE1 depth[1];
        depth[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2);

        params[eDepthTexture].InitAsDescriptorTable(_countof(depth), depth);

        CD3DX12_DESCRIPTOR_RANGE1 indices[1];
        indices[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

        params[eLightIndexBuffer].InitAsDescriptorTable(_countof(indices), indices);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(_countof(params), params, 0, nullptr, kBinningPassRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto cs = context.mConfig.bundle.get_shader_bytecode("forward_plus_tiling.cs");

        const D3D12_COMPUTE_PIPELINE_STATE_DESC kPipeline = {
            .pRootSignature = pipeline.signature.get(),
            .CS = CD3DX12_SHADER_BYTECODE(cs.data(), cs.size()),
        };

        auto device = context.getDevice();
        SM_ASSERT_HR(device->CreateComputePipelineState(&kPipeline, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void ecs::lightBinning(
    flecs::world &world,
    graph::FrameGraph &graph,
    graph::Handle &indices,
    graph::Handle depth,
    graph::Handle pointLightData,
    graph::Handle spotLightData,
    flecs::entity camera
)
{
    graph::PassBuilder pass = graph.compute("Light Binning");
    pass.side_effects(true);

    const graph::ResourceInfo lightIndexInfo = {
        .size = graph::ResourceSize::buffer(sizeof(uint) * MAX_LIGHTS)
    };

    indices = pass.create(lightIndexInfo, "Light Indices", graph::Usage::eBufferWrite)
        .override_uav({
            .Format = DXGI_FORMAT_R32_UINT,
            .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = MAX_LIGHTS,
            },
        })
        .override_srv({
            .Format = DXGI_FORMAT_R32_UINT,
            .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = MAX_LIGHTS,
            },
        });

    pass.read(depth, "Depth", graph::Usage::eTextureRead);
    pass.read(pointLightData, "Point Light Data", graph::Usage::eBufferRead);
    pass.read(spotLightData, "Spot Light Data", graph::Usage::eBufferRead);

    auto& data = graph.newDeviceData([](render::IDeviceContext& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        createLightBinningPipeline(info.pipeline, context);

        return info;
    });

    pass.bind([=, &data](graph::RenderContext& ctx) {
        auto& [context, graph, _, commands] = ctx;

        ID3D12Resource *pointLightHandle = graph.resource(pointLightData);
        ID3D12Resource *spotLightHandle = graph.resource(spotLightData);

        auto depthTexture = graph.srv(depth);
        auto depthTextureHandle = context.mSrvPool.gpu_handle(depthTexture);

        auto lightIndices = graph.uav(indices);
        auto lightIndicesHandle = context.mSrvPool.gpu_handle(lightIndices);

        const ecs::ViewportDeviceData *vpd = camera.get<ecs::ViewportDeviceData>();

        commands->SetComputeRootSignature(data.pipeline.signature.get());
        commands->SetPipelineState(data.pipeline.pso.get());

        commands->SetComputeRootConstantBufferView(eViewportBuffer, vpd->getDeviceAddress());

        commands->SetComputeRootShaderResourceView(ePointLightData, pointLightHandle->GetGPUVirtualAddress());
        commands->SetComputeRootShaderResourceView(eSpotLightData, spotLightHandle->GetGPUVirtualAddress());

        commands->SetComputeRootDescriptorTable(eDepthTexture, depthTextureHandle);

        commands->SetComputeRootDescriptorTable(eLightIndexBuffer, lightIndicesHandle);

        commands->Dispatch(TILE_SIZE, TILE_SIZE, 1);
    });
}
