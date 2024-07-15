#include "stdafx.hpp"

#include "draw/draw.hpp"

#include "world/ecs.hpp"

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

    pipeline.rename("forward_plus_tiling");
}

void ecs::lightBinning(
    DrawData& dd,
    graph::Handle &indices,
    graph::Handle depth,
    graph::Handle pointLightData,
    graph::Handle spotLightData
)
{
    const world::ecs::Camera *info = dd.camera.get<world::ecs::Camera>();

    uint tileCount = draw::computeTileCount(info->window, TILE_SIZE);
    uint tileIndexCount = tileCount * LIGHT_INDEX_BUFFER_STRIDE;

    const graph::ResourceInfo lightIndexInfo = graph::ResourceInfo::arrayOf<light_index_t>(tileIndexCount);

    graph::PassBuilder pass = dd.graph.compute("Light Binning");

    indices = pass.create(lightIndexInfo, "Light Indices", graph::Usage::eBufferWrite)
        .override_srv({
            .Format = render::bufferFormatOf<light_index_t>(),
            .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = tileIndexCount,
            },
        });

    pass.read(depth, "Depth", graph::Usage::eTextureRead);
    pass.read(pointLightData, "Point Light Volumes", graph::Usage::eBufferRead);
    pass.read(spotLightData, "Spot Light Volumes", graph::Usage::eBufferRead);

    auto& data = dd.graph.newDeviceData([](render::IDeviceContext& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        createLightBinningPipeline(info.pipeline, context);

        return info;
    });

    pass.bind([=, camera = dd.camera, &data](graph::RenderContext& ctx) {
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

        uint2 gridSize = computeGridSize(info->window, TILE_SIZE);
        commands->Dispatch(gridSize.x, gridSize.y, 1);
    });
}
