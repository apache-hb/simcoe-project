#include "stdafx.hpp"

#include "draw/draw.hpp"

#include "render/core/render.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>
#include <directx/d3dx12_barriers.h>

#include <array>

using namespace sm;
using namespace sm::math;

namespace blit {
    struct Vertex {
        float2 position;
        float2 uv;
    };

    static constexpr auto kScreenQuad = std::to_array<Vertex>({
        { { -1.f, -1.f }, { 0.f, 1.f } },
        { { -1.f,  1.f }, { 0.f, 0.f } },
        { {  1.f, -1.f }, { 1.f, 1.f } },
        { {  1.f,  1.f }, { 1.f, 0.f } },
    });
}

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kPostRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

static void create_blit_pipeline(render::Pipeline& pipeline, render::IDeviceContext& context) {
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 params[1];

        // TODO: apply D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC
        //       once this is done in a seperate command list
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
        params[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        const D3D12_STATIC_SAMPLER_DESC kSampler = {
            .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            .AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            .AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            .AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
            .MinLOD = 0.f,
            .MaxLOD = D3D12_FLOAT32_MAX,
            .ShaderRegister = 0,
            .RegisterSpace = 0,
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
        };

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(_countof(params), params, 1, &kSampler, kPostRootFlags);

        context.serialize_root_signature(pipeline.signature, desc);
    }

    {
        auto ps = context.mConfig.bundle.get_shader_bytecode("blit.ps");
        auto vs = context.mConfig.bundle.get_shader_bytecode("blit.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(blit::Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(blit::Vertex, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kDesc = {
            .pRootSignature = pipeline.signature.get(),
            .VS = CD3DX12_SHADER_BYTECODE(vs.data(), vs.size()),
            .PS = CD3DX12_SHADER_BYTECODE(ps.data(), ps.size()),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .InputLayout = { kInputElements, _countof(kInputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { context.getSwapChainFormat() },
            .SampleDesc = { 1, 0 },
        };
        auto device = context.getDevice();

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

static void create_screen_quad(render::Resource& quad, render::IDeviceContext& context) {
    context.upload([&] {
        quad = context.uploadBufferData(sm::Span(blit::kScreenQuad), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    });
}

void draw::blit(graph::FrameGraph& graph, graph::Handle target, graph::Handle source, const render::Viewport& viewport) {
    graph::PassBuilder pass = graph.graphics("Blit");
    pass.write(target, "Target", graph::Usage::eRenderTarget);
    pass.read(source, "Image", graph::Usage::ePixelShaderResource);

    auto& data = graph.newDeviceData([](render::IDeviceContext& context) {
        struct {
            render::Pipeline pipeline;
            render::Resource quad;

            D3D12_VERTEX_BUFFER_VIEW getQuadView() const {
                D3D12_VERTEX_BUFFER_VIEW view = {
                    .BufferLocation = quad.getDeviceAddress(),
                    .SizeInBytes = sizeof(blit::Vertex) * blit::kScreenQuad.size(),
                    .StrideInBytes = sizeof(blit::Vertex),
                };

                return view;
            }
        } info;

        create_blit_pipeline(info.pipeline, context);
        create_screen_quad(info.quad, context);

        return info;
    });

    pass.bind([target, source, viewport, &data](graph::RenderContext& ctx) {
        auto& context = ctx.context;
        auto& graph = ctx.graph;
        auto *cmd = ctx.commands;
        auto rtv = graph.rtv(target);
        auto srv = graph.srv(source);

        auto rtv_cpu = context.mRtvPool.cpu_handle(rtv);
        auto srv_gpu = context.mSrvPool.gpu_handle(srv);

        D3D12_VERTEX_BUFFER_VIEW view = data.getQuadView();

        cmd->SetGraphicsRootSignature(*data.pipeline.signature);
        cmd->SetPipelineState(*data.pipeline.pso);

        cmd->RSSetViewports(1, viewport.viewport());
        cmd->RSSetScissorRects(1, viewport.scissor());

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, nullptr);

        cmd->ClearRenderTargetView(rtv_cpu, render::kColourBlack.data(), 0, nullptr);
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        cmd->IASetVertexBuffers(0, 1, &view);

        cmd->SetGraphicsRootDescriptorTable(0, srv_gpu);
        cmd->DrawInstanced(4, 1, 0, 0);
    });
}
