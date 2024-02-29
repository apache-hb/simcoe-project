#include "render/render.hpp"

#include "core/format.hpp" // IWYU pragma: export

#include "render/draw.hpp" // IWYU pragma: export

#include "math/colour.hpp"

#include "DDSTextureLoader12.h"

#include "d3dx12/d3dx12_core.h"
#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_barriers.h"

using namespace sm;
using namespace sm::render;

static uint get_swapchain_flags(const Instance& instance) {
    return instance.tearing_support() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
}

static logs::Severity get_severity(MessageSeverity sev) {
    using enum MessageSeverity::Inner;

    switch (sev.as_enum()) {
    case eMessage: return logs::Severity::eTrace;
    case eInfo: return logs::Severity::eInfo;
    case eWarning: return logs::Severity::eWarning;
    case eError: return logs::Severity::eError;
    case eCorruption: return logs::Severity::eFatal;

    default: return logs::Severity::eError;
    }
}

void Context::on_info(
    D3D12_MESSAGE_CATEGORY category,
    D3D12_MESSAGE_SEVERITY severity,
    D3D12_MESSAGE_ID id,
    LPCSTR desc,
    void *user)
{
    MessageCategory c{category};
    MessageSeverity s{severity};
    MessageID message{id};
    auto *ctx = static_cast<Context *>(user);
    auto& sink = ctx->mSink;

    sink.log(get_severity(s), "{} {}: {}", c, message, desc);
}

void Context::enable_info_queue() {
    if (Result hr = mDevice.query(&mInfoQueue)) {
        mInfoQueue->RegisterMessageCallback(&on_info, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &mCookie);
    } else {
        mSink.warn("failed to query info queue: {}", hr);
    }
}

void Context::enable_debug_layer(bool gbv, bool rename) {
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&mDebug))) {
        mDebug->EnableDebugLayer();

        Object<ID3D12Debug3> debug3;
        if (gbv) {
            if (mDebug.query(&debug3)) debug3->SetEnableGPUBasedValidation(true);
            else mSink.warn("failed to get debug3 interface: {}", hr);
        }

        Object<ID3D12Debug5> debug5;
        if (rename) {
            if (mDebug.query(&debug5)) debug5->SetEnableAutoName(true);
            else mSink.warn("failed to get debug5 interface: {}", hr);
        }
    } else {
        mSink.warn("failed to get debug interface: {}", hr);
    }
}

void Context::enable_dred() {
    Object<ID3D12DeviceRemovedExtendedDataSettings> dred;
    if (Result hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dred))) {
        dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
        dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    } else {
        mSink.warn("failed to query dred settings: {}", hr);
    }
}

void Context::query_root_signature_version() {
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature = {
        .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1,
    };

    if (Result hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature, sizeof(feature)); !hr) {
        mSink.warn("failed to query root signature version: {}", hr);
        mRootSignatureVersion = RootSignatureVersion::eVersion_1_0;
    } else {
        mRootSignatureVersion = feature.HighestVersion;
        mSink.info("root signature version: {}", mRootSignatureVersion);
    }
}

void Context::serialize_root_signature(Object<ID3D12RootSignature>& signature, const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc) {
    Blob serialized;
    Blob error;
    if (Result hr = D3DX12SerializeVersionedRootSignature(&desc, mRootSignatureVersion.as_facade(), &serialized, &error); !hr) {
        mSink.error("failed to serialize root signature: {}", hr);
        mSink.error("| error: {}", error.as_string());
        SM_ASSERT_HR(hr);
    }

    SM_ASSERT_HR(mDevice->CreateRootSignature(0, serialized.data(), serialized.size(), IID_PPV_ARGS(&signature)));
}

void Context::create_device(size_t index) {
    auto flags = mInstance.flags();
    auto& adapter = mInstance.get_adapter(index);

    if (flags.test(DebugFlags::eDeviceDebugLayer))
        enable_debug_layer(flags.test(DebugFlags::eGpuValidation), flags.test(DebugFlags::eAutoName));

    if (flags.test(DebugFlags::eDeviceRemovedInfo))
        enable_dred();

    auto fl = get_feature_level();

    mAdapterIndex = index;
    if (Result hr = D3D12CreateDevice(adapter.get(), fl.as_facade(), IID_PPV_ARGS(&mDevice)); !hr) {
        mSink.error("failed to create device `{}` at feature level `{}`", adapter.name(), fl, hr);
        mSink.error("| hresult: {}", hr);
        mSink.error("falling back to warp adapter...");

        mAdapterIndex = mInstance.warp_adapter_index();
        auto& warp_adapter = mInstance.get_adapter(mAdapterIndex);
        SM_ASSERT_HR(D3D12CreateDevice(warp_adapter.get(), fl.as_facade(), IID_PPV_ARGS(&mDevice)));
    }

    mSink.info("device created: {}", adapter.name());

    if (flags.test(DebugFlags::eInfoQueue))
        enable_info_queue();

    query_root_signature_version();

    mSink.info("| feature level: {}", fl);
    mSink.info("| flags: {}", flags);
}

static constexpr D3D12MA::ALLOCATOR_FLAGS kAllocatorFlags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED | D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;

void Context::create_allocator() {
    const D3D12MA::ALLOCATOR_DESC kAllocatorDesc = {
        .Flags = kAllocatorFlags,
        .pDevice = mDevice.get(),
        .pAdapter = *mInstance.get_adapter(mAdapterIndex),
    };

    SM_ASSERT_HR(D3D12MA::CreateAllocator(&kAllocatorDesc, &mAllocator));
}

void Context::reset_direct_commands(ID3D12PipelineState *pso) {
    auto& allocator = mFrames[mFrameIndex].mCommandAllocator;
    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(mCommandList->Reset(allocator.get(), pso));
}

void Context::create_copy_queue() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_COPY,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mCopyQueue)));

    SM_ASSERT_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mCopyAllocator)));
    SM_ASSERT_HR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, mCopyAllocator.get(), nullptr, IID_PPV_ARGS(&mCopyCommands)));
    SM_ASSERT_HR(mCopyCommands->Close());
}

void Context::reset_copy_commands() {
    SM_ASSERT_HR(mCopyAllocator->Reset());
    SM_ASSERT_HR(mCopyCommands->Reset(*mCopyAllocator, nullptr));
}

void Context::create_copy_fence() {
    SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mCopyFence)));
    mCopyFenceValue = 1;

    SM_ASSERT_WIN32(mCopyFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr));
}

void Context::create_pipeline() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mDirectQueue)));

    bool tearing = mInstance.tearing_support();
    const DXGI_SWAP_CHAIN_DESC1 kSwapChainDesc = {
        .Width = mSwapChainSize.width,
        .Height = mSwapChainSize.height,
        .Format = mSwapChainFormat,
        .SampleDesc = { 1, 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = mSwapChainLength,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = get_swapchain_flags(mInstance),
    };

    auto& factory = mInstance.factory();
    HWND hwnd = mConfig.window.get_handle();

    Object<IDXGISwapChain1> swapchain1;
    SM_ASSERT_HR(factory->CreateSwapChainForHwnd(*mDirectQueue, hwnd, &kSwapChainDesc, nullptr, nullptr, &swapchain1));

    if (tearing)
    {
        SM_ASSERT_HR(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    }

    SM_ASSERT_HR(swapchain1.query(&mSwapChain));
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    create_rtv_heap();

    {
        uint count = min_srv_heap_size();
        DescriptorHeap srv_heap;
        SM_ASSERT_HR(create_descriptor_heap(srv_heap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, count, true));

        mSrvAllocator.set_heap(std::move(srv_heap));
    }

    {
        SM_ASSERT_HR(create_descriptor_heap(mDsvHeap, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false));
    }

    create_depth_stencil();

    mFrames.resize(mSwapChainLength);

    update_scene_viewport();
    create_render_targets();
    create_frame_allocators();

    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].mFenceValue = 0;
    }
}

void Context::create_rtv_heap() {
    mRtvHeap.reset();

    uint count = min_rtv_heap_size();

    SM_ASSERT_HR(create_descriptor_heap(mRtvHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, count, false));
}

bool Context::resize_rtv_heap(uint length) {
    if (mRtvHeap.mCapacity >= length) return false;

    mRtvHeap.reset();

    SM_ASSERT_HR(create_descriptor_heap(mRtvHeap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, length, false));

    return true;
}

Result Context::create_descriptor_heap(DescriptorHeap& heap, D3D12_DESCRIPTOR_HEAP_TYPE type, uint capacity, bool shader_visible) {
    const D3D12_DESCRIPTOR_HEAP_DESC kHeapDesc = {
        .Type = type,
        .NumDescriptors = capacity,
        .Flags = shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
    };

    heap.mDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(type);
    heap.mCapacity = capacity;
    return mDevice->CreateDescriptorHeap(&kHeapDesc, IID_PPV_ARGS(&heap));
}

static const D3D12_HEAP_PROPERTIES kDefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
static const D3D12_HEAP_PROPERTIES kUploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

void Context::create_depth_stencil() {
    const D3D12_CLEAR_VALUE kClearValue = {
        .Format = mDepthFormat,
        .DepthStencil = { 1.0f, 0 },
    };

    const auto kDepthBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(mDepthFormat, mSceneSize.width, mSceneSize.height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    SM_ASSERT_HR(create_resource(mDepthStencil, D3D12_HEAP_TYPE_DEFAULT, kDepthBufferDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &kClearValue));

    const D3D12_DEPTH_STENCIL_VIEW_DESC kDesc = {
        .Format = mDepthFormat,
        .ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
    };

    mDevice->CreateDepthStencilView(*mDepthStencil.mResource, &kDesc, mDsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Context::create_render_targets() {
    CTASSERTF(mSwapChainLength <= DXGI_MAX_SWAP_CHAIN_BUFFERS, "too many swap chain buffers (%u > %u)", mSwapChainLength, DXGI_MAX_SWAP_CHAIN_BUFFERS);

    for (uint i = 0; i < mSwapChainLength; i++) {
        auto& backbuffer = mFrames[i].mRenderTarget;
        auto rtv = mRtvHeap.cpu_descriptor_handle(frame_rtv_index(i));
        SM_ASSERT_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
        mDevice->CreateRenderTargetView(*backbuffer, nullptr, rtv);
    }
}

void Context::create_frame_allocators() {
    for (uint i = 0; i < mSwapChainLength; i++) {
        SM_ASSERT_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrames[i].mCommandAllocator)));
    }
}

void Context::update_scene_viewport() {
    mSceneViewport = Viewport{mSceneSize};
}

Result Context::create_resource(Resource& resource, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE *clear) {
    const D3D12MA::ALLOCATION_DESC kAllocDesc = {
        .HeapType = heap,
    };

    return mAllocator->CreateResource(&kAllocDesc, &desc, state, clear, &resource.mAllocation, IID_PPV_ARGS(&resource.mResource));
}

Result Context::load_dds_texture(Object<ID3D12Resource>& texture, sm::Vector<D3D12_SUBRESOURCE_DATA>& mips, const char *name) {
    auto data = mConfig.bundle.get_texture(name);
    Result hr = DirectX::LoadDDSTextureFromMemory(
        mDevice.get(),
        data.data(),
        data.size_bytes(),
        &texture,
        mips
    );

    if (hr.success()) {
        texture.rename(name);
        return S_OK;
    }

    mSink.error("failed to load dds texture: {}", hr);
    return hr;
}

void Context::copy_buffer(Object<ID3D12GraphicsCommandList1>& list, Resource& dst, Resource& src, size_t size) {
    list->CopyBufferRegion(*dst.mResource, 0, *src.mResource, 0, size);
}

static const D3D12_ROOT_SIGNATURE_FLAGS kPrimitiveRootFlags
    = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
    | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

void Context::create_primitive_pipeline() {
    {
        // mvp matrix
        CD3DX12_ROOT_PARAMETER1 params[1];
        params[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc;
        desc.Init_1_1(1, params, 0, nullptr, kPrimitiveRootFlags);

        serialize_root_signature(mPrimitivePipeline.mRootSignature, desc);
    }

    {
        auto ps = mConfig.bundle.get_shader_bytecode("primitive.ps");
        auto vs = mConfig.bundle.get_shader_bytecode("primitive.vs");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(draw::Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOUR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, offsetof(draw::Vertex, colour), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kDesc = {
            .pRootSignature = mPrimitivePipeline.mRootSignature.get(),
            .VS = CD3DX12_SHADER_BYTECODE(vs.data(), vs.size()),
            .PS = CD3DX12_SHADER_BYTECODE(ps.data(), ps.size()),
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
            .DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
            .InputLayout = { kInputElements, _countof(kInputElements) },
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
            .NumRenderTargets = 1,
            .RTVFormats = { mSceneFormat },
            .DSVFormat = mDepthFormat,
            .SampleDesc = { 1, 0 },
        };

        SM_ASSERT_HR(mDevice->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&mPrimitivePipeline.mPipelineState)));
    }
}

void Context::destroy_primitive_pipeline() {
    mPrimitivePipeline.reset();
}

static constexpr draw::MeshInfo kInfo = {
    .type = draw::MeshType::eCube,
    .cube = {
        .width = 1.f,
        .height = 1.f,
        .depth = 1.f,
    },
};

void Context::init_scene() {
    mPrimitives.push_back(create_mesh(kInfo, float3(1.f, 0.f, 0.f)));
}

void Context::create_scene() {
    for (auto& primitive : mPrimitives) {
        primitive = create_mesh(primitive.mInfo, float3(1.f, 0.f, 0.f));
    }
}

void Context::destroy_scene() {
    for (auto& primitive : mPrimitives) {
        primitive.mVertexBuffer.reset();
        primitive.mIndexBuffer.reset();
    }
}

Context::Primitive Context::create_mesh(const draw::MeshInfo& info, const float3& colour) {
    auto mesh = draw::primitive(info);

    uint32_t col = pack_colour(float4(colour, 1.0f));

    for (auto& v : mesh.vertices) {
        v.colour = col;
    }

    Resource vbo_upload;
    Resource ibo_upload;

    const uint kVertexBufferSize = mesh.vertices.size() * sizeof(draw::Vertex);
    const uint kIndexBufferSize = mesh.indices.size() * sizeof(uint16);

    const auto kVertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kVertexBufferSize);
    const auto kIndexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kIndexBufferSize);

    Primitive primitive = {
        .mInfo = info,
        .mIndexCount = int_cast<uint>(mesh.indices.size()),
    };

    SM_ASSERT_HR(create_resource(vbo_upload, D3D12_HEAP_TYPE_UPLOAD, kVertexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ));
    SM_ASSERT_HR(create_resource(ibo_upload, D3D12_HEAP_TYPE_UPLOAD, kIndexBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ));

    D3D12_RANGE read{0, 0};

    {
        void *data;
        SM_ASSERT_HR(vbo_upload.map(&read, &data));
        std::memcpy(data, mesh.vertices.data(), kVertexBufferSize);
        vbo_upload.unmap(&read);
    }

    {
        void *data;
        SM_ASSERT_HR(ibo_upload.map(&read, &data));
        std::memcpy(data, mesh.indices.data(), kIndexBufferSize);
        ibo_upload.unmap(&read);
    }

    SM_ASSERT_HR(create_resource(primitive.mVertexBuffer, D3D12_HEAP_TYPE_DEFAULT, kVertexBufferDesc, D3D12_RESOURCE_STATE_COMMON));
    SM_ASSERT_HR(create_resource(primitive.mIndexBuffer, D3D12_HEAP_TYPE_DEFAULT, kIndexBufferDesc, D3D12_RESOURCE_STATE_COMMON));

    reset_direct_commands();
    reset_copy_commands();

    copy_buffer(mCopyCommands, primitive.mVertexBuffer, vbo_upload, kVertexBufferSize);
    copy_buffer(mCopyCommands, primitive.mIndexBuffer, ibo_upload, kIndexBufferSize);

    const D3D12_RESOURCE_BARRIER kBarriers[2] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            *primitive.mVertexBuffer.mResource,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        ),
        CD3DX12_RESOURCE_BARRIER::Transition(
            *primitive.mIndexBuffer.mResource,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_INDEX_BUFFER
        )
    };

    mCommandList->ResourceBarrier(_countof(kBarriers), kBarriers);

    SM_ASSERT_HR(mCopyCommands->Close());
    SM_ASSERT_HR(mCommandList->Close());

    ID3D12CommandList *copy_lists[] = { mCopyCommands.get() };
    mCopyQueue->ExecuteCommandLists(1, copy_lists);
    mCopyQueue->Signal(*mCopyFence, mCopyFenceValue);

    ID3D12CommandList *direct_lists[] = { mCommandList.get() };
    mDirectQueue->Wait(*mCopyFence, mCopyFenceValue);
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    primitive.mVertexBufferView = {
        .BufferLocation = primitive.mVertexBuffer.get_gpu_address(),
        .SizeInBytes = kVertexBufferSize,
        .StrideInBytes = sizeof(draw::Vertex),
    };

    primitive.mIndexBufferView = {
        .BufferLocation = primitive.mIndexBuffer.get_gpu_address(),
        .SizeInBytes = kIndexBufferSize,
        .Format = DXGI_FORMAT_R16_UINT,
    };

    mUploads.emplace(Upload{ std::move(vbo_upload), mCopyFenceValue });
    mUploads.emplace(Upload{ std::move(ibo_upload), mCopyFenceValue });

    wait_for_gpu();
    flush_copy_queue();

    return primitive;
}

void Context::create_assets() {
    create_primitive_pipeline();
    create_blit_pipeline();

    SM_ASSERT_HR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, *mFrames[mFrameIndex].mCommandAllocator, nullptr, IID_PPV_ARGS(&mCommandList)));
    SM_ASSERT_HR(mCommandList->Close());

    SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFrames[mFrameIndex].mFenceValue += 1;

    SM_ASSERT_WIN32(mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr));
}

void Context::build_command_list() {
    mAllocator->SetCurrentFrameIndex(mFrameIndex);

    reset_direct_commands(*mPrimitivePipeline.mPipelineState);
    mCommandList->SetGraphicsRootSignature(*mPrimitivePipeline.mRootSignature);

    {
        /// scene setup

        const auto kSceneRtvHandle = mRtvHeap.cpu_descriptor_handle(scene_rtv_index());
        const auto kDsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
        mCommandList->RSSetViewports(1, &mSceneViewport.mViewport);
        mCommandList->RSSetScissorRects(1, &mSceneViewport.mScissorRect);
        mCommandList->OMSetRenderTargets(1, &kSceneRtvHandle, false, &kDsvHandle);

        mCommandList->ClearRenderTargetView(kSceneRtvHandle, kClearColour.data(), 0, nullptr);
        mCommandList->ClearDepthStencilView(kDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        /// draw primitives

        auto [width, height] = mSceneSize.as<float>();

        for (auto& primitive : mPrimitives) {
            const float4x4 model = primitive.mTransform.matrix().transpose();
            const float4x4 mvp = camera.mvp(width / height, model).transpose();
            mCommandList->SetGraphicsRoot32BitConstants(0, 16, mvp.data(), 0);

            mCommandList->IASetVertexBuffers(0, 1, &primitive.mVertexBufferView);
            mCommandList->IASetIndexBuffer(&primitive.mIndexBufferView);

            mCommandList->DrawIndexedInstanced(primitive.mIndexCount, 1, 0, 0, 0);
        }
    }

    {
        auto& [backbuffer, allocator, _] = mFrames[mFrameIndex];

        const D3D12_RESOURCE_BARRIER kBeginPostBarriers[] = {
            // transition backbuffer from present to render target
            CD3DX12_RESOURCE_BARRIER::Transition(
                *backbuffer,
                D3D12_RESOURCE_STATE_PRESENT,
                D3D12_RESOURCE_STATE_RENDER_TARGET
            ),
            // transition scene target from render target to pixel shader resource
            CD3DX12_RESOURCE_BARRIER::Transition(
                *mSceneTarget.mResource,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
            )
        };

        mCommandList->ResourceBarrier(_countof(kBeginPostBarriers), kBeginPostBarriers);

        ID3D12DescriptorHeap *heaps[] = { mSrvAllocator.get() };
        mCommandList->SetDescriptorHeaps(_countof(heaps), heaps);

        /// post
        mCommandList->SetGraphicsRootSignature(*mBlitPipeline.mRootSignature);
        mCommandList->SetPipelineState(*mBlitPipeline.mPipelineState);

        mCommandList->RSSetViewports(1, &mPresentViewport.mViewport);
        mCommandList->RSSetScissorRects(1, &mPresentViewport.mScissorRect);

        const auto rtv_handle = mRtvHeap.cpu_descriptor_handle(frame_rtv_index(mFrameIndex));
        mCommandList->OMSetRenderTargets(1, &rtv_handle, false, nullptr);

        mCommandList->ClearRenderTargetView(rtv_handle, kColourBlack.data(), 0, nullptr);
        mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        mCommandList->IASetVertexBuffers(0, 1, &mScreenQuad.mVertexBufferView);

        const auto scene_srv_handle = mSrvAllocator.gpu_descriptor_handle(mSceneTargetSrvIndex);

        mCommandList->SetGraphicsRootDescriptorTable(0, scene_srv_handle);
        mCommandList->DrawInstanced(4, 1, 0, 0);

        /// imgui
        render_imgui();

        const D3D12_RESOURCE_BARRIER kEndPostBarriers[] = {
            // transition backbuffer from render target to present
            CD3DX12_RESOURCE_BARRIER::Transition(
                *backbuffer,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT
            ),
            // transition scene target from pixel shader resource to render target
            CD3DX12_RESOURCE_BARRIER::Transition(
                *mSceneTarget.mResource,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET
            )
        };

        mCommandList->ResourceBarrier(_countof(kEndPostBarriers), kEndPostBarriers);
    }

    SM_ASSERT_HR(mCommandList->Close());
}

void Context::destroy_device() {
    // release frame resources
    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].mRenderTarget.reset();
        mFrames[i].mCommandAllocator.reset();
    }

    // depth stencil
    mDepthStencil.reset();

    // release sync objects
    mFence.reset();
    mCopyFence.reset();

    // assets
    destroy_scene();
    destroy_scene_target();
    destroy_screen_quad();

    // pipeline state
    destroy_primitive_pipeline();
    destroy_blit_pipeline();

    // copy commands
    mCopyCommands.reset();
    mCopyAllocator.reset();
    mCopyQueue.reset();

    // direct commands
    mDirectQueue.reset();
    mCommandList.reset();

    // allocator
    mAllocator.reset();

    // descriptor heaps
    mRtvHeap.reset();
    mDsvHeap.reset();
    mSrvAllocator.reset();

    // swapchain
    mSwapChain.reset();

    // device
    mDebug.reset();
    mInfoQueue.reset();
    mDevice.reset();
}

Context::Context(const RenderConfig& config)
    : mConfig(config)
    , mSink(config.logger)
    , mInstance({ config.flags, config.preference, config.logger })
    , mSwapChainFormat(config.swapchain_format)
    , mSwapChainSize(config.swapchain_size)
    , mSwapChainLength(config.swapchain_length)
    , mSceneFormat(config.scene_format)
    , mDepthFormat(config.depth_format)
    , mSceneSize(config.scene_size)
    , mPresentViewport(mSwapChainSize)
    , mSceneViewport(mSceneSize)
{ }

void Context::create() {
    size_t index = mConfig.flags.test(DebugFlags::eWarpAdapter)
        ? mInstance.warp_adapter_index()
        : mConfig.adapter_index;

    create_device(index);
    create_allocator();
    create_copy_queue();
    create_copy_fence();
    create_pipeline();
    create_assets();

    mSceneTargetSrvIndex = mSrvAllocator.allocate();

    create_screen_quad();
    create_scene_target();
    init_scene();
    create_imgui();
}

void Context::destroy() {
    flush_copy_queue();
    wait_for_gpu();

    destroy_imgui();

    mSrvAllocator.release(mSceneTargetSrvIndex);
    mSceneTargetSrvIndex = UINT_MAX;

    SM_ASSERT_WIN32(CloseHandle(mCopyFenceEvent));
    SM_ASSERT_WIN32(CloseHandle(mFenceEvent));
}

bool Context::update() {
    return update_imgui();
}

void Context::render() {
    build_command_list();

    ID3D12CommandList *lists[] = { mCommandList.get() };
    mDirectQueue->ExecuteCommandLists(1, lists);

    bool tearing = mInstance.tearing_support();
    uint flags = tearing ? DXGI_PRESENT_ALLOW_TEARING : 0;

    SM_ASSERT_HR(mSwapChain->Present(0, flags));

    move_to_next_frame();
}

void Context::update_adapter(size_t index) {
    if (index == mAdapterIndex) return;

    wait_for_gpu();
    destroy_imgui_backend();
    destroy_device();

    create_device(index);
    create_allocator();
    create_copy_queue();
    create_copy_fence();
    create_pipeline();
    create_assets();

    create_scene_target();
    create_screen_quad();
    create_scene();

    create_imgui_backend();
}

void Context::update_swapchain_length(uint length) {
    wait_for_gpu();

    uint64 current = mFrames[mFrameIndex].mFenceValue;

    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].mRenderTarget.reset();
        mFrames[i].mCommandAllocator.reset();
    }

    const uint flags = get_swapchain_flags(mInstance);
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(length, mSwapChainSize.width, mSwapChainSize.height, mSwapChainFormat, flags));
    mSwapChainLength = length;

    mFrames.resize(length);
    for (uint i = 0; i < length; i++) {
        mFrames[i].mFenceValue = current;
    }

    if (resize_rtv_heap(min_rtv_heap_size())) {
        create_scene_render_target();
    }
    create_render_targets();
    create_frame_allocators();

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void Context::resize_swapchain(math::uint2 size) {
    wait_for_gpu();

    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].mRenderTarget.reset();
        mFrames[i].mFenceValue = mFrames[mFrameIndex].mFenceValue;
    }

    const uint flags = get_swapchain_flags(mInstance);
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(mSwapChainLength, size.width, size.height, mSwapChainFormat, flags));
    mSwapChainSize = size;

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    update_display_viewport();
    create_render_targets();
}

void Context::resize_draw(math::uint2 size) {
    wait_for_gpu();

    mDepthStencil.reset();
    destroy_scene_target();

    mSceneSize = size;

    update_scene_viewport();
    update_display_viewport();
    create_scene_target();
    create_depth_stencil();
}

void Context::move_to_next_frame() {
    const uint64 current = mFrames[mFrameIndex].mFenceValue;
    SM_ASSERT_HR(mDirectQueue->Signal(*mFence, current));

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    if (mFence->GetCompletedValue() < mFrames[mFrameIndex].mFenceValue) {
        SM_ASSERT_HR(mFence->SetEventOnCompletion(mFrames[mFrameIndex].mFenceValue, mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mFrames[mFrameIndex].mFenceValue = current + 1;
}

void Context::wait_for_gpu() {
    const uint64 current = mFrames[mFrameIndex].mFenceValue++;
    SM_ASSERT_HR(mDirectQueue->Signal(*mFence, current));

    SM_ASSERT_HR(mFence->SetEventOnCompletion(current, mFenceEvent));
    WaitForSingleObject(mFenceEvent, INFINITE);

    // TODO: currently this is correct as all copies are followed by
    // a direct state transition. but at some point that may not be the case
    // come back here when that happens.

    const uint64 completed = mFence->GetCompletedValue();
    while (!mUploads.is_empty()) {
        auto& upload = mUploads.top();
        if (completed < upload.value) break;

        mUploads.pop();
    }
}

void Context::flush_copy_queue() {
    const uint64 current = mCopyFenceValue++;
    SM_ASSERT_HR(mCopyQueue->Signal(*mCopyFence, current));

    const uint64 completed = mCopyFence->GetCompletedValue();

    if (completed < current) {
        SM_ASSERT_HR(mCopyFence->SetEventOnCompletion(current, mCopyFenceEvent));
        WaitForSingleObject(mCopyFenceEvent, INFINITE);
    }
}
