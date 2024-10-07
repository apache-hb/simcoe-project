#include "stdafx.hpp"

#include "draw/draw.hpp"

#include "world/ecs.hpp"

using namespace sm;
using namespace sm::draw;

enum {
    eViewportBuffer, // register(b0)
    eObjectBuffer, // register(b1)

    eTextureArray, // register(t0, space1)

    ePointLightVolumeData, // register(t0)
    eSpotLightVolumeData, // register(t1)
    ePointLightParams, // register(t2)
    eSpotLightParams, // register(t3)

    eLightIndexBuffer, // register(t4)

    eBindingCount
};

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kOpaquePassRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

static void createOpaquePipeline(
    render::Pipeline& pipeline,
    render::IDeviceContext& context,
    DXGI_FORMAT colour,
    DXGI_FORMAT depth
)
{
    {
        CD3DX12_ROOT_PARAMETER1 params[eBindingCount];
        params[eViewportBuffer].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
        params[eObjectBuffer].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

        CD3DX12_DESCRIPTOR_RANGE1 texture[1];
        texture[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, UINT_MAX, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

        params[eTextureArray].InitAsDescriptorTable(_countof(texture), texture);

        params[ePointLightVolumeData].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_PIXEL);
        params[eSpotLightVolumeData].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_PIXEL);
        params[ePointLightParams].InitAsShaderResourceView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_PIXEL);
        params[eSpotLightParams].InitAsShaderResourceView(3, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_DESCRIPTOR_RANGE1 indices[1];
        indices[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);

        params[eLightIndexBuffer].InitAsDescriptorTable(_countof(indices), indices, D3D12_SHADER_VISIBILITY_PIXEL);

        CD3DX12_STATIC_SAMPLER_DESC samplers[1];
        samplers[0].Init(
            0, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            0, 16,
            D3D12_COMPARISON_FUNC_NEVER,
            D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
            0, D3D12_FLOAT32_MAX, D3D12_SHADER_VISIBILITY_PIXEL
        );

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(_countof(params), params, _countof(samplers), samplers, kOpaquePassRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto vs = context.mConfig.bundle->get_shader_bytecode("forward_plus_opaque.vs");
        auto ps = context.mConfig.bundle->get_shader_bytecode("forward_plus_opaque.ps");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(world::Vertex, position) },

            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(world::Vertex, normal) },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(world::Vertex, uv) },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(world::Vertex, tangent) },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kPipeline = {
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
            .RTVFormats = { colour },
            .DSVFormat = depth,
            .SampleDesc = { 1, 0 },
        };

        auto device = context.getDevice();
        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kPipeline, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void ecs::forwardPlusOpaque(
    WorldData& wd,
    DrawData& dd,
    graph::Handle lightIndices,
    graph::Handle pointLightVolumeData,
    graph::Handle spotLightVolumeData,
    graph::Handle pointLightData,
    graph::Handle spotLightData,
    graph::Handle& target,
    graph::Handle& depth
)
{
    const world::ecs::Camera *info = wd.camera.get<world::ecs::Camera>();

    const graph::ResourceInfo targetInfo = graph::ResourceInfo::tex2d(info->window, info->colour)
        .clearColour(render::kClearColour);

    const graph::ResourceInfo depthInfo = graph::ResourceInfo::tex2d(info->window, info->depth)
        .clearDepthStencil(1.f, 0);

    graph::PassBuilder pass = dd.graph.graphics(fmt::format("Forward+ Opaque ({})", wd.camera.name().c_str()))
        .hasSideEffects();

    target = pass.create(targetInfo, "Target", graph::Usage::eRenderTarget);
    depth = pass.create(depthInfo, "Depth", graph::Usage::eDepthWrite);

    pass.read(lightIndices, "Light Indices", graph::Usage::ePixelShaderResource)
        .withStates(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    pass.read(pointLightVolumeData, "Point Light Volume Data", graph::Usage::ePixelShaderResource)
        .withStates(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    pass.read(spotLightVolumeData, "Spot Light Volume Data", graph::Usage::ePixelShaderResource)
        .withStates(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    pass.read(pointLightData, "Point Light Data", graph::Usage::ePixelShaderResource)
        .withStates(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
    pass.read(spotLightData, "Spot Light Data", graph::Usage::ePixelShaderResource)
        .withStates(D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

    auto& data = dd.graph.newDeviceData([colour = info->colour, depth = info->depth](render::IDeviceContext& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        createOpaquePipeline(info.pipeline, context, colour, depth);

        return info;
    });

    pass.bind([=, objectDrawData = wd.objectDrawData, camera = wd.camera, &data](graph::RenderContext& ctx) {
        auto& [context, graph, _, commands] = ctx;

        const world::ecs::Camera *it = camera.get<world::ecs::Camera>();
        const ecs::ViewportDeviceData *dd = camera.get<ecs::ViewportDeviceData>();

        auto rtv = graph.rtv(target);
        auto dsv = graph.dsv(depth);

        auto rtvHostHandle = context.mRtvPool.cpu_handle(rtv);
        auto dsvHostHandle = context.mDsvPool.cpu_handle(dsv);

        ID3D12DescriptorHeap *srvHeap = context.mSrvPool.get();
        ID3D12Resource *pointLightVolumeHandle = graph.resource(pointLightVolumeData);
        ID3D12Resource *spotLightVolumeHandle = graph.resource(spotLightVolumeData);
        ID3D12Resource *pointLightHandle = graph.resource(pointLightData);
        ID3D12Resource *spotLightHandle = graph.resource(spotLightData);

        D3D12_GPU_DESCRIPTOR_HANDLE lightIndicesHandle = context.mSrvPool.gpu_handle(graph.srv(lightIndices));

        render::Viewport viewport{it->window};

        commands->SetGraphicsRootSignature(*data.pipeline.signature);
        commands->SetPipelineState(*data.pipeline.pso);

        commands->RSSetViewports(1, viewport.viewport());
        commands->RSSetScissorRects(1, viewport.scissor());

        commands->OMSetRenderTargets(1, &rtvHostHandle, false, &dsvHostHandle);
        commands->ClearRenderTargetView(rtvHostHandle, render::kClearColour.data(), 0, nullptr);
        commands->ClearDepthStencilView(dsvHostHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commands->SetGraphicsRootConstantBufferView(eViewportBuffer, dd->getDeviceAddress());

        commands->SetGraphicsRootDescriptorTable(eTextureArray, srvHeap->GetGPUDescriptorHandleForHeapStart());

        commands->SetGraphicsRootShaderResourceView(ePointLightVolumeData, pointLightVolumeHandle->GetGPUVirtualAddress());
        commands->SetGraphicsRootShaderResourceView(eSpotLightVolumeData, spotLightVolumeHandle->GetGPUVirtualAddress());
        commands->SetGraphicsRootShaderResourceView(ePointLightParams, pointLightHandle->GetGPUVirtualAddress());
        commands->SetGraphicsRootShaderResourceView(eSpotLightParams, spotLightHandle->GetGPUVirtualAddress());

        commands->SetGraphicsRootDescriptorTable(eLightIndexBuffer, lightIndicesHandle);

        objectDrawData.iter([&](flecs::iter& it, const ecs::ObjectDeviceData *dd, const render::ecs::IndexBuffer *ibo, const render::ecs::VertexBuffer *vbo) {
            for (auto i : it) {
                commands->SetGraphicsRootConstantBufferView(eObjectBuffer, dd[i].getDeviceAddress());
                commands->IASetVertexBuffers(0, 1, &vbo[i].view);
                commands->IASetIndexBuffer(&ibo[i].view);

                commands->DrawIndexedInstanced(ibo[i].length, 1, 0, 0, 0);
            }
        });
    });
}
