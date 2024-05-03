#include "stdafx.hpp"

#include "world/ecs.hpp"

#include "draw/draw.hpp"

using namespace sm;
using namespace sm::draw;
using namespace sm::world;

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kPrePassRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

enum {
    eViewportBuffer, // register(b0)

    eObjectBuffer, // register(b1)

    eBindingCount
};

static void createDepthPassPipeline(
    render::Pipeline& pipeline,
    render::IDeviceContext& context,
    DXGI_FORMAT depth)
{
    {
        CD3DX12_ROOT_PARAMETER1 params[eBindingCount];
        params[eViewportBuffer].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);

        params[eObjectBuffer].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);

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
            .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
            .InputLayout = { kInputElements, _countof(kInputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .DSVFormat = depth,
            .SampleDesc = { 1, 0 },
        };
        auto device = context.getDevice();

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kPipeline, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void draw::ecs::depthPrePass(
    DrawData& dd,
    graph::Handle &depthTarget
)
{
    const world::ecs::Camera *info = dd.camera.get<world::ecs::Camera>();

    const graph::ResourceInfo depthTargetInfo = graph::ResourceInfo::tex2d(info->window, render::getDepthTextureFormat(info->depth))
        .clearDepthStencil(1.f, 0, info->depth);

    graph::PassBuilder pass = dd.graph.graphics(fmt::format("Forward+ Depth Prepass ({})", dd.camera.name().c_str()));

    depthTarget = pass.create(depthTargetInfo, "Depth", graph::Usage::eDepthWrite)
        .override_srv({
            .Format = render::getShaderViewFormat(info->depth),
            .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
            .Texture2D = {
                .MostDetailedMip = 0,
                .MipLevels = 1,
            },
        })
        .override_dsv({
            .Format = info->depth,
            .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        });

    auto& data = dd.graph.newDeviceData([depth = info->depth](render::IDeviceContext& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        createDepthPassPipeline(info.pipeline, context, depth);

        return info;
    });

    pass.bind([depthTarget, objectDrawData = dd.objectDrawData, camera = dd.camera, &data](graph::RenderContext& ctx) {
        auto& [context, graph, _, commands] = ctx;

        auto dsv = graph.dsv(depthTarget);
        auto dsvHostHandle = context.mDsvPool.cpu_handle(dsv);

        const ecs::ViewportDeviceData *vpd = camera.get<ecs::ViewportDeviceData>();

        const world::ecs::Camera *info = camera.get<world::ecs::Camera>();
        render::Viewport vp{info->window};

        commands->SetGraphicsRootSignature(data.pipeline.signature.get());
        commands->SetPipelineState(data.pipeline.pso.get());

        commands->RSSetViewports(1, &vp.mViewport);
        commands->RSSetScissorRects(1, &vp.mScissorRect);

        commands->OMSetRenderTargets(0, nullptr, false, &dsvHostHandle);
        commands->ClearDepthStencilView(dsvHostHandle, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commands->SetGraphicsRootConstantBufferView(eViewportBuffer, vpd->getDeviceAddress());

        objectDrawData.iter([&](flecs::iter& it, const ecs::ObjectDeviceData *dd, const render::ecs::IndexBuffer *ib, const render::ecs::VertexBuffer *vb) {
            for (auto i : it) {
                commands->SetGraphicsRootConstantBufferView(eObjectBuffer, dd[i].getDeviceAddress());

                commands->IASetVertexBuffers(0, 1, &vb[i].view);
                commands->IASetIndexBuffer(&ib[i].view);

                commands->DrawIndexedInstanced(ib[i].length, 1, 0, 0, 0);
            }
        });
    });
}
