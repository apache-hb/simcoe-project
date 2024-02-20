#include "render/render.hpp"

#include "d3dx12/d3dx12_core.h"
#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_barriers.h"

using namespace sm;
using namespace sm::render;

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

void Context::create_blit_pipeline() {
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

        serialize_root_signature(mBlitPipeline.mRootSignature, desc);
    }

    {
        auto ps = mConfig.bundle.get_shader_bytecode("blit.ps");
        auto vs = mConfig.bundle.get_shader_bytecode("blit.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(blit::Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(blit::Vertex, uv), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kDesc = {
            .pRootSignature = mBlitPipeline.mRootSignature.get(),
            .VS = CD3DX12_SHADER_BYTECODE(vs.data(), vs.size()),
            .PS = CD3DX12_SHADER_BYTECODE(ps.data(), ps.size()),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .InputLayout = { kInputElements, _countof(kInputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { mConfig.swapchain_format },
            .SampleDesc = { 1, 0 },
        };

        SM_ASSERT_HR(mDevice->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&mBlitPipeline.mPipelineState)));
    }
}

void Context::destroy_blit_pipeline() {
    mBlitPipeline.reset();
}

void Context::create_screen_quad() {
    const auto kBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(blit::kScreenQuad));
    const auto kVertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(blit::kScreenQuad));

    Resource upload;

    SM_ASSERT_HR(create_resource(upload, D3D12_HEAP_TYPE_UPLOAD, kBufferDesc, D3D12_RESOURCE_STATE_COPY_SOURCE));

    SM_ASSERT_HR(create_resource(mScreenQuad.mVertexBuffer, D3D12_HEAP_TYPE_DEFAULT, kVertexBufferDesc, D3D12_RESOURCE_STATE_COMMON));

    void *data;
    D3D12_RANGE read{0, 0};
    SM_ASSERT_HR(upload.map(&read, &data));
    std::memcpy(data, blit::kScreenQuad, sizeof(blit::kScreenQuad));
    upload.unmap(&read);

    reset_direct_commands();
    reset_copy_commands();

    copy_buffer(mCopyCommands, mScreenQuad.mVertexBuffer, upload, sizeof(blit::kScreenQuad));

    const D3D12_RESOURCE_BARRIER kBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        *mScreenQuad.mVertexBuffer.mResource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );

    mCommandList->ResourceBarrier(1, &kBarrier);

    SM_ASSERT_HR(mCopyCommands->Close());
    SM_ASSERT_HR(mCommandList->Close());

    ID3D12CommandList *copy_lists[] = { mCopyCommands.get() };
    mCopyQueue->ExecuteCommandLists(1, copy_lists);
    mCopyQueue->Signal(*mCopyFence, mCopyFenceValue);

    ID3D12CommandList *direct_lists[] = { mCommandList.get() };
    mDirectQueue->Wait(*mCopyFence, mCopyFenceValue);
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    mUploads.emplace(Upload{ std::move(upload), mCopyFenceValue });

    wait_for_gpu();
    flush_copy_queue();

    mScreenQuad.mVertexBufferView = {
        .BufferLocation = mScreenQuad.mVertexBuffer.get_gpu_address(),
        .SizeInBytes = sizeof(blit::kScreenQuad),
        .StrideInBytes = sizeof(blit::Vertex),
    };
}

void Context::destroy_screen_quad() {
    mScreenQuad.mVertexBuffer.reset();
}

void Context::create_scene_render_target() {
    /// create rtv
    const auto rtv_handle = mRtvHeap.cpu_descriptor_handle(scene_rtv_index());
    mDevice->CreateRenderTargetView(mSceneTarget.mResource.get(), nullptr, rtv_handle);
}

void Context::create_scene_target() {
    CTASSERT(!mSceneTarget.mResource.is_valid());

    const auto kTargetDesc = CD3DX12_RESOURCE_DESC::Tex2D(mConfig.swapchain_format, mDrawSize.x, mDrawSize.y, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    const D3D12_CLEAR_VALUE kClear = {
        .Format = mConfig.swapchain_format,
        .Color = { kClearColour.r, kClearColour.g, kClearColour.b, kClearColour.a },
    };

    SM_ASSERT_HR(create_resource(mSceneTarget, D3D12_HEAP_TYPE_DEFAULT, kTargetDesc, D3D12_RESOURCE_STATE_RENDER_TARGET, &kClear));

    create_scene_render_target();

    /// create srv
    const auto srv_handle = mSrvHeap.cpu_descriptor_handle(eDescriptorScene);
    mDevice->CreateShaderResourceView(mSceneTarget.mResource.get(), nullptr, srv_handle);
}

void Context::destroy_scene_target() {
    mSceneTarget.reset();
}

void Context::update_display_viewport() {
    auto [renderWidth, renderHeight] = mDrawSize.as<float>();
    auto [displayWidth, displayHeight] = mSwapChainSize.as<float>();

    auto widthRatio = renderWidth / displayWidth;
    auto heightRatio = renderHeight / displayHeight;

    float x = 1.f;
    float y = 1.f;

    if (widthRatio < heightRatio) {
        x = widthRatio / heightRatio;
    } else {
        y = heightRatio / widthRatio;
    }

    float viewport_x = displayWidth * (1.f - x) / 2.f;
    float viewport_y = displayHeight * (1.f - y) / 2.f;
    float viewport_width = x * displayWidth;
    float viewport_height = y * displayHeight;

    mPresentViewport.mViewport = {
        .TopLeftX = viewport_x,
        .TopLeftY = viewport_y,
        .Width = viewport_width,
        .Height = viewport_height,
        .MinDepth = 0.f,
        .MaxDepth = 1.f,
    };

    mPresentViewport.mScissorRect = {
        .left = LONG(viewport_x),
        .top = LONG(viewport_y),
        .right = LONG(viewport_x + viewport_width),
        .bottom = LONG(viewport_y + viewport_height),
    };
}
