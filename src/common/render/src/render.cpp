#include "render/render.hpp"

#include "core/format.hpp" // IWYU pragma: export

#include "render/draw.hpp" // IWYU pragma: export
#include "archive/io.hpp"

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

void Context::create_copy_queue() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_COPY,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mCopyQueue)));

    SM_ASSERT_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mCopyAllocator)));
    SM_ASSERT_HR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, mCopyAllocator.get(), nullptr, IID_PPV_ARGS(&mCopyCommands)));
    SM_ASSERT_HR(mCopyCommands->Close());
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
    mSwapChainSize = mConfig.swapchain_size;
    mSwapChainLength = mConfig.swapchain_length;
    const DXGI_SWAP_CHAIN_DESC1 kSwapChainDesc = {
        .Width = mSwapChainSize.width,
        .Height = mSwapChainSize.height,
        .Format = mConfig.swapchain_format,
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

    create_rtv_heap(mSwapChainLength);

    {
        const D3D12_DESCRIPTOR_HEAP_DESC kHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = 1,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        };

        SM_ASSERT_HR(mDevice->CreateDescriptorHeap(&kHeapDesc, IID_PPV_ARGS(&mSrvHeap)));
        mSrvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    mFrames.resize(mSwapChainLength);

    update_viewport_scissor();
    create_render_targets();
    create_frame_allocators();

    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].mFenceValue = 0;
    }
}

void Context::create_rtv_heap(uint count) {
    mRtvHeap.reset();

    const D3D12_DESCRIPTOR_HEAP_DESC kHeapDesc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = count,
    };

    SM_ASSERT_HR(mDevice->CreateDescriptorHeap(&kHeapDesc, IID_PPV_ARGS(&mRtvHeap)));
    mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void Context::create_render_targets() {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (uint i = 0; i < mSwapChainLength; i++) {
        auto& backbuffer = mFrames[i].mRenderTarget;
        SM_ASSERT_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&backbuffer)));
        mDevice->CreateRenderTargetView(*backbuffer, nullptr, rtvHandle);
        rtvHandle.Offset(1, mRtvDescriptorSize);
    }
}

void Context::create_frame_allocators() {
    for (uint i = 0; i < mSwapChainLength; i++) {
        SM_ASSERT_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrames[i].mCommandAllocator)));
    }
}

void Context::update_viewport_scissor() {
    auto [width, height] = mSwapChainSize.as<float>();
    mViewport = {
        .TopLeftX = 0.f,
        .TopLeftY = 0.f,
        .Width = width,
        .Height = height,
        .MinDepth = 0.f,
        .MaxDepth = 1.f,
    };

    mScissorRect = {
        .left = 0,
        .top = 0,
        .right = (LONG)width,
        .bottom = (LONG)height,
    };
}

static sm::Vector<uint8> load_shader_bytecode(Sink& sink, const char *path) {
    auto file = Io::file(path, eAccessRead);
    if (auto err = file.error()) {
        sink.error("failed to open {}: {}", file.name(), err);
        return {};
    }

    sm::Vector<uint8> data;
    auto size = file.size();
    data.resize(size);
    if (file.read_bytes(data.data(), size) != size) {
        sink.error("failed to read {} bytes from {}: {}", size, file.name(), file.error());
        return {};
    }

    return data;
}

void Context::create_pipeline_state() {
    {
        CD3DX12_ROOT_SIGNATURE_DESC desc;
        desc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        Blob signature;
        Blob error;
        if (Result hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error); !hr) {
            mSink.error("failed to serialize root signature: {}", hr);
            mSink.error("| error: {}", error.as_string());
            SM_ASSERT_HR(hr);
        }

        SM_ASSERT_HR(mDevice->CreateRootSignature(0, signature.data(), signature.size(), IID_PPV_ARGS(&mRootSignature)));
    }

    {
        auto ps = load_shader_bytecode(mSink, "build/bundle/shaders/simple.ps.cso");
        auto vs = load_shader_bytecode(mSink, "build/bundle/shaders/simple.vs.cso");

        constexpr D3D12_INPUT_ELEMENT_DESC kInputElements[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(draw::Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOUR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(draw::Vertex, colour), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        const D3D12_GRAPHICS_PIPELINE_STATE_DESC kDesc = {
            .pRootSignature = mRootSignature.get(),
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

        SM_ASSERT_HR(mDevice->CreateGraphicsPipelineState(&kDesc, IID_PPV_ARGS(&mPipelineState)));
    }
}

Result Context::create_resource(Resource& resource, D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state) {
    const D3D12MA::ALLOCATION_DESC kAllocDesc = {
        .HeapType = heap,
    };

    return mAllocator->CreateResource(&kAllocDesc, &desc, state, nullptr, &resource.mAllocation, IID_PPV_ARGS(&resource.mHandle));
}

static const D3D12_HEAP_PROPERTIES kDefaultHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
static const D3D12_HEAP_PROPERTIES kUploadHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

void Context::create_triangle() {
    auto& allocator = mFrames[mFrameIndex].mCommandAllocator;
    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(mCommandList->Reset(allocator.get(), *mPipelineState));

    SM_ASSERT_HR(mCopyAllocator->Reset());
    SM_ASSERT_HR(mCopyCommands->Reset(mCopyAllocator.get(), nullptr));

    Resource upload;

    mVertexBuffer.reset();
    auto [width, height] = mSwapChainSize.as<float>();
    const float kAspectRatio = width / height;
    const draw::Vertex kTriangle[] = {
        { { 0.f, 0.25f * kAspectRatio, 0.f }, { 1.f, 0.f, 0.f, 1.f } },
        { { 0.25f, -0.25f * kAspectRatio, 0.f }, { 0.f, 1.f, 0.f, 1.f } },
        { { -0.25f, -0.25f * kAspectRatio, 0.f }, { 0.f, 0.f, 1.f, 1.f } },
    };

    const uint kBufferSize = sizeof(kTriangle);
    const auto kBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(kBufferSize);

    SM_ASSERT_HR(create_resource(upload, D3D12_HEAP_TYPE_UPLOAD, kBufferDesc, D3D12_RESOURCE_STATE_COMMON));

    void *data;
    D3D12_RANGE read{0, 0};
    SM_ASSERT_HR(upload.map(&read, &data));
    std::memcpy(data, kTriangle, kBufferSize);
    upload.unmap(&read);

    SM_ASSERT_HR(create_resource(mVertexBuffer, D3D12_HEAP_TYPE_DEFAULT, kBufferDesc, D3D12_RESOURCE_STATE_COMMON));

    const auto kBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        *mVertexBuffer.mHandle,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    );

    mCopyCommands->CopyBufferRegion(*mVertexBuffer.mHandle, 0, *upload.mHandle, 0, kBufferSize);
    mCommandList->ResourceBarrier(1, &kBarrier);

    SM_ASSERT_HR(mCopyCommands->Close());
    SM_ASSERT_HR(mCommandList->Close());

    // once we're done copying, signal the fence so the direct queue
    // can transition the resource
    ID3D12CommandList *copy_lists[] = { mCopyCommands.get() };
    mCopyQueue->ExecuteCommandLists(1, copy_lists);
    mCopyQueue->Signal(*mCopyFence, mCopyFenceValue);

    // wait for copy queue to finish before transitioning the resource
    ID3D12CommandList *direct_lists[] = { mCommandList.get() };
    mDirectQueue->Wait(*mCopyFence, mCopyFenceValue);
    mDirectQueue->ExecuteCommandLists(1, direct_lists);

    const D3D12_VERTEX_BUFFER_VIEW kBufferView = {
        .BufferLocation = mVertexBuffer.get_gpu_address(),
        .SizeInBytes = kBufferSize,
        .StrideInBytes = sizeof(draw::Vertex),
    };

    mVertexBufferView = kBufferView;

    wait_for_gpu();
    flush_copy_queue();
}

void Context::create_assets() {
    create_pipeline_state();

    SM_ASSERT_HR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, *mFrames[mFrameIndex].mCommandAllocator, nullptr, IID_PPV_ARGS(&mCommandList)));
    SM_ASSERT_HR(mCommandList->Close());

    SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFrames[mFrameIndex].mFenceValue += 1;

    SM_ASSERT_WIN32(mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr));

    create_triangle();
}

void Context::build_command_list() {
    mAllocator->SetCurrentFrameIndex(mFrameIndex);

    auto& [backbuffer, allocator, _] = mFrames[mFrameIndex];

    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(mCommandList->Reset(allocator.get(), *mPipelineState));

    mCommandList->SetGraphicsRootSignature(*mRootSignature);
    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    const auto kIntoRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
        *backbuffer,
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    mCommandList->ResourceBarrier(1, &kIntoRenderTarget);

    const auto kRtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), (int)mFrameIndex, mRtvDescriptorSize);

    mCommandList->OMSetRenderTargets(1, &kRtvHandle, false, nullptr);

    constexpr math::float4 kClearColour = { 0.0f, 0.2f, 0.4f, 1.0f };
    mCommandList->ClearRenderTargetView(kRtvHandle, kClearColour.data(), 0, nullptr);
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    mCommandList->DrawInstanced(3, 1, 0, 0);

    ID3D12DescriptorHeap *heaps[] = { mSrvHeap.get() };
    mCommandList->SetDescriptorHeaps(1, heaps);

    render_imgui();

    const auto kIntoPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        *backbuffer,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    mCommandList->ResourceBarrier(1, &kIntoPresent);

    SM_ASSERT_HR(mCommandList->Close());
}

void Context::destroy_device() {
    // release frame resources
    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].mRenderTarget.reset();
        mFrames[i].mCommandAllocator.reset();
    }

    // release sync objects
    mFence.reset();
    mCopyFence.reset();

    // assets
    mVertexBuffer.reset();

    // pipeline state
    mRootSignature.reset();
    mPipelineState.reset();

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
    mSrvHeap.reset();

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

    create_imgui();
}

void Context::destroy() {
    wait_for_gpu();

    destroy_imgui();

    SM_ASSERT_WIN32(CloseHandle(mFenceEvent));
}

void Context::update() {
    update_imgui();
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
    destroy_imgui_device();
    destroy_device();

    create_device(index);
    mSink.info("adapter changed to `{}`", mInstance.get_adapter(index).name());
    create_allocator();
    mSink.info("allocator created");
    create_copy_queue();
    mSink.info("copy queue created");
    create_copy_fence();
    mSink.info("copy fence created");
    create_pipeline();
    mSink.info("pipeline created");
    create_assets();
    mSink.info("assets created");

    create_imgui_device();
    mSink.info("imgui device created");
    update_imgui();
}

void Context::update_swapchain_length(uint length) {
    wait_for_gpu();

    uint64 current = mFrames[mFrameIndex].mFenceValue;

    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].mRenderTarget.reset();
        mFrames[i].mCommandAllocator.reset();
    }

    const uint flags = get_swapchain_flags(mInstance);
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(length, mSwapChainSize.width, mSwapChainSize.height, mConfig.swapchain_format, flags));
    mSwapChainLength = length;

    mFrames.resize(length);
    for (uint i = 0; i < length; i++) {
        mFrames[i].mFenceValue = current;
    }

    create_rtv_heap(length);
    create_render_targets();
    create_frame_allocators();

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void Context::resize(math::uint2 size) {
    wait_for_gpu();

    for (uint i = 0; i < mSwapChainLength; i++) {
        mFrames[i].mRenderTarget.reset();
        mFrames[i].mFenceValue = mFrames[mFrameIndex].mFenceValue;
    }

    const uint flags = get_swapchain_flags(mInstance);
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(mSwapChainLength, size.width, size.height, mConfig.swapchain_format, flags));
    mSwapChainSize = size;

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    update_viewport_scissor();
    create_render_targets();
    create_triangle();
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
}

void Context::flush_copy_queue() {
    const uint64 current = mCopyFenceValue++;
    SM_ASSERT_HR(mCopyQueue->Signal(*mCopyFence, current));

    if (mCopyFence->GetCompletedValue() < current) {
        SM_ASSERT_HR(mCopyFence->SetEventOnCompletion(current, mCopyFenceEvent));
        WaitForSingleObject(mCopyFenceEvent, INFINITE);
    }
}
