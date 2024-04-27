#include "stdafx.hpp"

#include "world/ecs.hpp"

#include "draw/draw.hpp"

#include "math/format.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>

using namespace sm;

using namespace sm::math::literals;

static const D3D12_ROOT_SIGNATURE_FLAGS kPrimitiveRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS
    ;

enum {
    eViewportBuffer, // register(b0)

    eObjectBuffer, // register(b1)

    eBindingCount
};

static void create_primitive_pipeline(
    render::IDeviceContext& context,
    render::Pipeline& pipeline,
    DXGI_FORMAT colour,
    DXGI_FORMAT depth)
{
    {
        // viewport data
        CD3DX12_ROOT_PARAMETER1 params[eBindingCount];
        params[eViewportBuffer].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);

        // transform matrix
        params[eObjectBuffer].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(_countof(params), params,0, nullptr, kPrimitiveRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto ps = context.mConfig.bundle.get_shader_bytecode("object.ps");
        auto vs = context.mConfig.bundle.get_shader_bytecode("object.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(world::Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(world::Vertex, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kPipelineDesc = {
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

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kPipelineDesc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

void draw::ecs::opaque(flecs::world& world, graph::FrameGraph& graph, graph::Handle& target, graph::Handle& depth, flecs::entity camera) {
#if 0
    static flecs::query updateObjectData
        = world.query_builder<
            ecs::ObjectDeviceData,
            const world::ecs::Position,
            const world::ecs::Rotation,
            const world::ecs::Scale
        >()
        // select the world position
        .term_at(2).second<world::ecs::World>()
        .build();
#endif

    static flecs::query drawObjectData = world.query<
        const ecs::ObjectDeviceData,
        const render::ecs::IndexBuffer,
        const render::ecs::VertexBuffer
    >();

    const world::ecs::Camera *it = camera.get<world::ecs::Camera>();

    graph::PassBuilder pass = graph.graphics(fmt::format("Opaque {}", camera.name().c_str()));

    graph::ResourceInfo depthInfo = {
        .size = graph::ResourceSize::tex2d(it->window),
        .format = it->depth,
        .clear = graph::Clear::depthStencil(1.f, 0, it->depth),
    };

    graph::ResourceInfo targetInfo = {
        .size = graph::ResourceSize::tex2d(it->window),
        .format = it->colour,
        .clear = graph::Clear::colour(render::kClearColour, it->colour),
    };

    target = pass.create(targetInfo, "Target", graph::Usage::eRenderTarget);
    depth = pass.create(depthInfo, "Depth", graph::Usage::eDepthWrite);

    auto& data = graph.device_data([it](render::IDeviceContext& context) {
        struct {
            render::Pipeline pipeline;
        } info;

        create_primitive_pipeline(context, info.pipeline, it->colour, it->depth);

        return info;
    });

    pass.bind([target, depth, camera, &data](graph::RenderContext& ctx) {
        auto& [device, graph, _, commands] = ctx;

        const world::ecs::Camera *it = camera.get<world::ecs::Camera>();
        const ecs::ViewportDeviceData *dd = camera.get<ecs::ViewportDeviceData>();

        auto rtv = graph.rtv(target);
        auto dsv = graph.dsv(depth);

        auto rtvHostHandle = device.mRtvPool.cpu_handle(rtv);
        auto dsvHostHandle = device.mDsvPool.cpu_handle(dsv);

        render::Viewport viewport{it->window};

        commands->SetGraphicsRootSignature(*data.pipeline.signature);
        commands->SetPipelineState(*data.pipeline.pso);

        commands->RSSetViewports(1, &viewport.mViewport);
        commands->RSSetScissorRects(1, &viewport.mScissorRect);

        commands->OMSetRenderTargets(1, &rtvHostHandle, false, &dsvHostHandle);
        commands->ClearRenderTargetView(rtvHostHandle, render::kClearColour.data(), 0, nullptr);
        commands->ClearDepthStencilView(dsvHostHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        commands->SetGraphicsRootConstantBufferView(eViewportBuffer, dd->getDeviceAddress());

#if 0
        updateObjectData.iter([&](flecs::iter& it, ecs::ObjectDeviceData *dd, const world::ecs::Position *position, const world::ecs::Rotation *rotation, const world::ecs::Scale *scale) {
            for (auto i : it) {
                // TODO: multiplying scale by -1 is wrong, figure it out when im less tired
                float4x4 model = math::float4x4::transform(position[i].position, rotation[i].rotation, -scale[i].scale);
                dd[i].update({ (model.transpose() * v * p).transpose() });
            }
        });
#endif

        drawObjectData.iter([&](flecs::iter& it, const ecs::ObjectDeviceData *dd, const render::ecs::IndexBuffer *ibo, const render::ecs::VertexBuffer *vbo) {
            for (auto i : it) {
                commands->SetGraphicsRootConstantBufferView(eObjectBuffer, dd[i].getDeviceAddress());
                commands->IASetVertexBuffers(0, 1, &vbo[i].view);
                commands->IASetIndexBuffer(&ibo[i].view);

                commands->DrawIndexedInstanced(ibo[i].length, 1, 0, 0, 0);
            }
        });
    });
}
