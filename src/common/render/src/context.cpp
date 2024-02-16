#include "render/render.hpp"
#include "common.hpp"

#include "logs/sink.inl" // IWYU pragma: keep
#include "core/reflect.hpp" // IWYU pragma: keep

#include "d3dx12/d3dx12_core.h"
#include "d3dx12/d3dx12_barriers.h"

using namespace sm;
using namespace sm::render;

Context::Context(RenderConfig config)
    : mSink(config.logger)
    , mInstance({ config.debug_flags, config.adapter_preference, config.logger })
    , mAdapterIndex(config.adapter_index)
    , mFeatureLevel(config.feature_level)
    , mFrames(config.frame_count)
    , mDirectCommandLists(CommandListType::eDirect, config.direct_command_pool_size)
    , mCopyCommandLists(CommandListType::eCopy, config.copy_command_pool_size)
    , mComputeCommandLists(CommandListType::eCompute, config.compute_command_pool_size)
    , mResources(config.resource_pool_size)
{
    create_device();
    query_root_signature_version();
    create_allocator();
    create_device_objects(config);
    create_swapchain(config);
    create_backbuffers(config);
}

Context::~Context() {
    flush_direct_queue();
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

void Context::create_device() {
    auto flags = mInstance.flags();
    auto& adapter = flags.test(DebugFlags::eWarpAdapter)
        ? mInstance.warp_adapter()
        : mInstance.get_adapter(mAdapterIndex);

    if (flags.test(DebugFlags::eDeviceDebugLayer))
        enable_debug_layer(flags.test(DebugFlags::eGpuValidation), flags.test(DebugFlags::eAutoName));

    if (flags.test(DebugFlags::eDeviceRemovedInfo))
        enable_dred();

    mAdapter = std::addressof(adapter);
    if (Result hr = D3D12CreateDevice(adapter.get(), mFeatureLevel.as_facade(), IID_PPV_ARGS(&mDevice)); !hr) {
        mSink.error("failed to create device `{}` at feature level `{}`", adapter.name(), mFeatureLevel, hr);
        mSink.error("| hresult: {}", hr);
        mSink.error("falling back to warp adapter...");

        auto& warp = mInstance.warp_adapter();
        SM_ASSERT_HR(D3D12CreateDevice(warp.get(), mFeatureLevel.as_facade(), IID_PPV_ARGS(&mDevice)));
        mAdapter = std::addressof(warp);
    }

    mSink.info("device created: {}", adapter.name());

    if (flags.test(DebugFlags::eInfoQueue))
        enable_info_queue();

    mSink.info("| feature level: {}", mFeatureLevel);
    mSink.info("| flags: {}", flags);
}

void Context::query_root_signature_version() {
    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature = {
        .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_2,
    };

    if (Result hr = mDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature, sizeof(feature))) {
        mSink.error("failed to query root signature version: {}", hr);
        mRootSignatureVersion = RootSignatureVersion::eVersion_1_0;
    } else {
        mRootSignatureVersion = feature.HighestVersion;
    }

    mSink.info("root signature version: {}", mRootSignatureVersion);
}

void Context::create_device_objects(const RenderConfig& config) {
    mDirectQueue.create(mDevice, CommandListType::eDirect);
    mCopyQueue.create(mDevice, CommandListType::eCopy);
    mComputeQueue.create(mDevice, CommandListType::eCompute);

    UINT rtv_heap_size = config.rtv_heap_size;
    UINT dsv_heap_size = config.dsv_heap_size;
    UINT srv_heap_size = config.srv_heap_size;

    if (rtv_heap_size < config.frame_count) {
        mSink.warn("rtv heap size `{}` is less than frame count `{}`, increasing...", rtv_heap_size, config.frame_count);
    }

    if (dsv_heap_size == 0) {
        mSink.warn("dsv heap size is zero, setting to 1...");
        dsv_heap_size = 1;
    }

    if (srv_heap_size == 0) {
        mSink.warn("srv heap size is zero, setting to 1...");
        srv_heap_size = 1;
    }

    mSink.info("| heap sizes: rtv={}, dsv={}, srv={}", rtv_heap_size, dsv_heap_size, srv_heap_size);

    mRenderTargetHeap.create(mDevice, DescriptorHeapType::eRTV, rtv_heap_size, false);
    mDepthStencilHeap.create(mDevice, DescriptorHeapType::eDSV, dsv_heap_size, false);
    mShaderResourceHeap.create(mDevice, DescriptorHeapType::eResource, srv_heap_size, true);

    mSink.info("| pools: direct={}, copy={}, compute={}", config.direct_command_pool_size, config.copy_command_pool_size, config.compute_command_pool_size);

    mDirectCommandLists.create(mDevice);
    mCopyCommandLists.create(mDevice);
    mComputeCommandLists.create(mDevice);

    mPresentFence.create(mDevice, "present");
    mDirectFence.create(mDevice, "direct");
    mCopyFence.create(mDevice, "copy");
}

static constexpr D3D12MA::ALLOCATOR_FLAGS kAllocatorFlags = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED | D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;

void Context::create_allocator() {
    D3D12MA::ALLOCATOR_DESC desc = {
        .Flags = kAllocatorFlags,
        .pDevice = mDevice.get(),
        .pAdapter = mAdapter->get(),
    };

    SM_ASSERT_HR(D3D12MA::CreateAllocator(&desc, &mAllocator));
}

void Context::create_swapchain(const RenderConfig& config) {
    auto& window = config.window;
    auto coords = window.get_client_coords();
    auto [width, height] = coords.size().as<UINT>();
    auto fmt = config.swapchain_format;

    DXGI_SWAP_CHAIN_DESC desc = {
        .BufferDesc = {
            .Width = width,
            .Height = height,
            .RefreshRate = { 60, 1 },
            .Format = fmt.as_facade(),
            .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            .Scaling = DXGI_MODE_SCALING_UNSPECIFIED,
        },
        .SampleDesc = { 1, 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = config.frame_count,
        .OutputWindow = window.get_handle(),
        .Windowed = TRUE,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
    };

    Object<IDXGISwapChain> swapchain;
    auto& factory = mInstance.factory();
    SM_ASSERT_HR(factory->CreateSwapChain(mDirectQueue.get(), &desc, &swapchain));

    SM_ASSERT_HR(swapchain.query(&mSwapChain));

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    mSink.info("swapchain created: {}x{}, format={}", width, height, fmt);
    mSink.info("| backbuffer count: {}", config.frame_count);
}

void Context::create_backbuffers(const RenderConfig& config) {
    mFrames.resize(config.frame_count);

    for (size_t i = 0; i < config.frame_count; ++i) {
        auto& frame = mFrames[i];
        SM_ASSERT_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&frame.buffer)));
        frame.rtv = mRenderTargetHeap.acquire();

        mDevice->CreateRenderTargetView(frame.buffer.get(), nullptr, mRenderTargetHeap.cpu(frame.rtv));

        frame.commands.create(mDevice, CommandListType::eDirect);
        frame.commands.close();
    }
}

void Context::flush_direct_queue() {
    auto value = mFrames[mFrameIndex].value++;
    mDirectQueue.signal(mPresentFence, value);
    mPresentFence.wait(value);
}

void Context::next_frame() {
    auto current_value = mFrames[mFrameIndex].value;
    mDirectQueue.signal(mPresentFence, current_value);

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
    mPresentFence.wait(mFrames[mFrameIndex].value);

    mFrames[mFrameIndex].value = current_value + 1;
}

void Context::begin_frame() {
    reclaim_live_objects(mPresentFence);
    mAllocator->SetCurrentFrameIndex(mFrameIndex);

    auto& frame = mFrames[mFrameIndex];

    frame.commands.reset();

    auto *cmd = frame.commands.get();

    const CD3DX12_RESOURCE_BARRIER into_render = CD3DX12_RESOURCE_BARRIER::Transition(
        frame.buffer.get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    cmd->ResourceBarrier(1, &into_render);

    const D3D12_CPU_DESCRIPTOR_HANDLE rtv = mRenderTargetHeap.cpu(frame.rtv);

    constexpr float kClearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    cmd->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    cmd->ClearRenderTargetView(rtv, kClearColor, 0, nullptr);

    const CD3DX12_RESOURCE_BARRIER into_present = CD3DX12_RESOURCE_BARRIER::Transition(
        frame.buffer.get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    cmd->ResourceBarrier(1, &into_present);

    frame.commands.close();
}

void Context::reclaim_direct_list(CommandList *list, Context& ctx) {
    list->reset();
    ctx.mDirectCommandLists.release(list);
}

void Context::end_frame() {
    sm::SmallVector<ID3D12CommandList*, 4> lists;
    lists.push_back(mFrames[mFrameIndex].commands.get());

    auto& pending_direct = mPendingLists[uint(CommandListType::eDirect)];

    // take all the pending submit lists and execute them
    for (auto *list : pending_direct)
        lists.push_back(list->get());

    mDirectQueue.execute((uint)lists.size(), lists.data());

    // move now executing commands into the reclaim queue
    auto& executing = mPendingObjects[&mPresentFence];
    uint64 value = mFrames[mFrameIndex].value;
    for (auto *list : pending_direct)
        executing.emplace(PendingObject{ kReclaimDirectList, list, value });

    // remove the submitted lists from the pending list
    pending_direct.clear();

    SM_ASSERT_HR(mSwapChain->Present(1, 0));

    next_frame();
}

CommandList& Context::acquire_copy_list() {
    CommandList *list = mCopyCommandLists.acquire();
    CTASSERTF(list, "Copy pool exhausted %zu", mCopyCommandLists.length());
    return *list;
}

void Context::submit_copy_list(CommandList &list) {
    list.close();
    mPendingLists[uint(CommandListType::eCopy)].push_back(&list);
}

CommandList& Context::acquire_direct_list() {
    CommandList *list = mDirectCommandLists.acquire();
    CTASSERTF(list, "Direct pool exhausted %zu", mDirectCommandLists.length());
    return *list;
}

void Context::submit_direct_list(CommandList& list) {
    list.close();
    mPendingLists[uint(CommandListType::eDirect)].push_back(&list);
}

void Context::reclaim_live_objects(Fence& fence) {
    auto& pending = mPendingObjects[&fence];
    auto now = fence.value();

    while (!pending.empty()) {
        if (auto& top = pending.top(); top.value() <= now) {
            top.dispose(*this);
            pending.pop();
        } else {
            break;
        }
    }
}

DeviceResource Context::create_resource(D3D12_HEAP_TYPE heap, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state) {
    const D3D12MA::ALLOCATION_DESC alloc = {
        .HeapType = heap,
    };

    DeviceResource res{};

    SM_ASSERT_HR(mAllocator->CreateResource(&alloc, &desc, state, nullptr, &res.allocation, IID_PPV_ARGS(&res.resource)));

    return res;
}

DeviceResource Context::create_staging_buffer(uint size) {
    const auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
    return create_resource(D3D12_HEAP_TYPE_UPLOAD, desc, D3D12_RESOURCE_STATE_COPY_SOURCE);
}

DeviceResource Context::create_buffer(uint size) {
    const auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
    return create_resource(D3D12_HEAP_TYPE_DEFAULT, desc, D3D12_RESOURCE_STATE_COPY_DEST);
}
