#include "stdafx.hpp"

#include "draw/draw.hpp"

#include "render/render.hpp"

#include <directx/d3dx12_core.h>
#include <directx/d3dx12_root_signature.h>
#include <directx/d3dx12_barriers.h>

using namespace sm;
using namespace sm::math;

namespace blit {
    struct Vertex {
        float2 position;
        float2 uv;
    };

    static constexpr Vertex kScreenQuad[] = {
        { { -1.f, -1.f }, { 0.f, 1.f } },
        { { -1.f,  1.f }, { 0.f, 0.f } },
        { {  1.f, -1.f }, { 1.f, 1.f } },
        { {  1.f,  1.f }, { 1.f, 0.f } },
    };
}

static constexpr D3D12_ROOT_SIGNATURE_FLAGS kPostRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

static void create_blit_pipeline(render::Pipeline& pipeline, render::Context& context) {
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
            .RTVFormats = { context.mSwapChainConfig.format },
            .SampleDesc = { 1, 0 },
        };
        auto device = context.getDevice();

        SM_ASSERT_HR(device->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&pipeline.pso)));
    }
}

static void create_screen_quad(render::Resource& quad, render::VertexBufferView& vbo, render::Context& context) {
    const auto kBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(blit::kScreenQuad));

    render::Resource upload;

    SM_ASSERT_HR(context.create_resource(upload, D3D12_HEAP_TYPE_UPLOAD, kBufferDesc, D3D12_RESOURCE_STATE_COPY_SOURCE));

    SM_ASSERT_HR(context.create_resource(quad, D3D12_HEAP_TYPE_DEFAULT, kBufferDesc, D3D12_RESOURCE_STATE_COMMON));

    void *data;
    D3D12_RANGE read{0, 0};
    SM_ASSERT_HR(upload.map(&read, &data));
    std::memcpy(data, blit::kScreenQuad, sizeof(blit::kScreenQuad));
    upload.unmap(&read);

    context.reset_direct_commands();
    context.reset_copy_commands();

    context.copy_buffer(*context.mCopyCommands, quad, upload, sizeof(blit::kScreenQuad));

    const D3D12_RESOURCE_BARRIER kBarriers[] = {
        // screen quad verts
        CD3DX12_RESOURCE_BARRIER::Transition(
            *quad.mResource,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        ),
    };

    context.mCommandList->ResourceBarrier(_countof(kBarriers), kBarriers);

    SM_ASSERT_HR(context.mCopyCommands->Close());
    SM_ASSERT_HR(context.mCommandList->Close());

    ID3D12CommandList *copy_lists[] = { context.mCopyCommands.get() };
    context.mCopyQueue->ExecuteCommandLists(1, copy_lists);

    SM_ASSERT_HR(context.mCopyQueue->Signal(*context.mCopyFence, context.mCopyFenceValue));

    ID3D12CommandList *direct_lists[] = { context.mCommandList.get() };
    SM_ASSERT_HR(context.mDirectQueue->Wait(*context.mCopyFence, context.mCopyFenceValue));
    context.mDirectQueue->ExecuteCommandLists(1, direct_lists);

    context.wait_for_gpu();
    context.flush_copy_queue();

    vbo = {
        .BufferLocation = quad.get_gpu_address(),
        .SizeInBytes = sizeof(blit::kScreenQuad),
        .StrideInBytes = sizeof(blit::Vertex),
    };
}

void draw::blit(graph::FrameGraph& graph, graph::Handle target, graph::Handle source, const render::Viewport& viewport) {
    graph::PassBuilder pass = graph.graphics("Blit");
    pass.write(target, "Target", graph::Usage::eRenderTarget);
    pass.read(source, "Image", graph::Usage::ePixelShaderResource);

    auto& data = graph.device_data([](render::Context& context) {
        struct {
            render::Pipeline pipeline;
            render::Resource quad;
            render::VertexBufferView vbo;
        } info;

        create_blit_pipeline(info.pipeline, context);
        create_screen_quad(info.quad, info.vbo, context);

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

        cmd->SetGraphicsRootSignature(*data.pipeline.signature);
        cmd->SetPipelineState(*data.pipeline.pso);

        cmd->RSSetViewports(1, &viewport.mViewport);
        cmd->RSSetScissorRects(1, &viewport.mScissorRect);

        cmd->OMSetRenderTargets(1, &rtv_cpu, false, nullptr);

        cmd->ClearRenderTargetView(rtv_cpu, render::kColourBlack.data(), 0, nullptr);
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        cmd->IASetVertexBuffers(0, 1, &data.vbo);

        cmd->SetGraphicsRootDescriptorTable(0, srv_gpu);
        cmd->DrawInstanced(4, 1, 0, 0);
    });
}
