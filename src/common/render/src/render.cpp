#include "render/render.hpp"

#include "core/format.hpp" // IWYU pragma: export

#include "math/math.hpp"

#include "d3dx12/d3dx12_root_signature.h"
#include "d3dx12/d3dx12_barriers.h"

using namespace sm;
using namespace sm::render;

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

    auto fl = get_feature_level();

    mAdapter = std::addressof(adapter);
    if (Result hr = D3D12CreateDevice(adapter.get(), fl.as_facade(), IID_PPV_ARGS(&mDevice)); !hr) {
        mSink.error("failed to create device `{}` at feature level `{}`", adapter.name(), fl, hr);
        mSink.error("| hresult: {}", hr);
        mSink.error("falling back to warp adapter...");

        auto& warp = mInstance.warp_adapter();
        SM_ASSERT_HR(D3D12CreateDevice(warp.get(), fl.as_facade(), IID_PPV_ARGS(&mDevice)));
        mAdapter = std::addressof(warp);
    }

    mSink.info("device created: {}", adapter.name());

    if (flags.test(DebugFlags::eInfoQueue))
        enable_info_queue();

    mSink.info("| feature level: {}", fl);
    mSink.info("| flags: {}", flags);
}

void Context::create_pipeline() {
    constexpr D3D12_COMMAND_QUEUE_DESC kQueueDesc = {
        .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
    };

    SM_ASSERT_HR(mDevice->CreateCommandQueue(&kQueueDesc, IID_PPV_ARGS(&mQueue)));

    mSwapChainSize = mConfig.swapchain_size;
    const DXGI_SWAP_CHAIN_DESC1 kSwapChainDesc = {
        .Width = mSwapChainSize.width,
        .Height = mSwapChainSize.height,
        .Format = mConfig.swapchain_format,
        .SampleDesc = { 1, 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = mConfig.swapchain_length,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
    };

    auto& factory = mInstance.factory();
    HWND hwnd = mConfig.window.get_handle();

    Object<IDXGISwapChain1> swapchain1;
    SM_ASSERT_HR(factory->CreateSwapChainForHwnd(mQueue.get(), hwnd, &kSwapChainDesc, nullptr, nullptr, &swapchain1));
    SM_ASSERT_HR(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

    SM_ASSERT_HR(swapchain1.query(&mSwapChain));
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    {
        const D3D12_DESCRIPTOR_HEAP_DESC kRtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
            .NumDescriptors = mConfig.swapchain_length,
        };

        SM_ASSERT_HR(mDevice->CreateDescriptorHeap(&kRtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

        mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    uint count = mConfig.swapchain_length;
    mAllocators.resize(count);
    mFenceValues.resize(count);
    mRenderTargets.resize(count);

    create_render_targets();
    create_frame_objects();
}

void Context::create_render_targets() {
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (uint i = 0; i < mConfig.swapchain_length; i++) {
        SM_ASSERT_HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mRenderTargets[i])));
        mDevice->CreateRenderTargetView(*mRenderTargets[i], nullptr, rtvHandle);
        rtvHandle.Offset(1, mRtvDescriptorSize);
    }
}

void Context::create_frame_objects() {
    mFenceValues.fill(0);

    for (uint i = 0; i < mConfig.swapchain_length; i++) {
        SM_ASSERT_HR(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mAllocators[i])));
    }
}

void Context::create_assets() {
    SM_ASSERT_HR(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mAllocators[mFrameIndex].get(), nullptr, IID_PPV_ARGS(&mCommandList)));

    SM_ASSERT_HR(mCommandList->Close());

    {
        SM_ASSERT_HR(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
        mFenceValues[mFrameIndex] += 1;

        SM_ASSERT_WIN32(mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr));
    }

    wait_for_gpu();
}

Context::Context(const RenderConfig& config)
    : mConfig(config)
    , mSink(config.logger)
    , mInstance({ config.flags, config.preference, config.logger })
{ }

void Context::create() {
    create_device();
    create_pipeline();
    create_assets();
}

void Context::destroy() {
    wait_for_gpu();

    SM_ASSERT_WIN32(CloseHandle(mFenceEvent));
}

void Context::update() {

}

void Context::render() {
    build_command_list();

    ID3D12CommandList *lists[] = { mCommandList.get() };
    mQueue->ExecuteCommandLists(1, lists);

    SM_ASSERT_HR(mSwapChain->Present(1, 0));

    move_to_next_frame();
}

void Context::resize(math::uint2 size) {
    wait_for_gpu();

    for (uint i = 0; i < mConfig.swapchain_length; i++) {
        mRenderTargets[i].reset();
        mFenceValues[i] = mFenceValues[mFrameIndex];
    }

    // TODO: DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
    SM_ASSERT_HR(mSwapChain->ResizeBuffers(mConfig.swapchain_length, size.width, size.height, mConfig.swapchain_format, 0));
    mSwapChainSize = size;

    create_render_targets();

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void Context::build_command_list() {
    auto& allocator = mAllocators[mFrameIndex];
    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(mCommandList->Reset(allocator.get(), nullptr));

    const auto kIntoRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(
        *mRenderTargets[mFrameIndex],
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );

    mCommandList->ResourceBarrier(1, &kIntoRenderTarget);

    const auto kRtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), (int)mFrameIndex, mRtvDescriptorSize);

    constexpr math::float4 kClearColour = { 0.0f, 0.2f, 0.4f, 1.0f };
    mCommandList->ClearRenderTargetView(kRtvHandle, kClearColour.data(), 0, nullptr);

    const auto kIntoPresent = CD3DX12_RESOURCE_BARRIER::Transition(
        *mRenderTargets[mFrameIndex],
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );

    mCommandList->ResourceBarrier(1, &kIntoPresent);

    SM_ASSERT_HR(mCommandList->Close());
}

void Context::move_to_next_frame() {
    const uint64 current = mFenceValues[mFrameIndex];
    SM_ASSERT_HR(mQueue->Signal(*mFence, current));

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    if (mFence->GetCompletedValue() < mFenceValues[mFrameIndex]) {
        SM_ASSERT_HR(mFence->SetEventOnCompletion(mFenceValues[mFrameIndex], mFenceEvent));
        WaitForSingleObjectEx(mFenceEvent, INFINITE, false);
    }

    mFenceValues[mFrameIndex] = current + 1;
}

void Context::wait_for_gpu() {
    SM_ASSERT_HR(mQueue->Signal(*mFence, mFenceValues[mFrameIndex]));

    SM_ASSERT_HR(mFence->SetEventOnCompletion(mFenceValues[mFrameIndex], mFenceEvent));
    WaitForSingleObjectEx(mFenceEvent, INFINITE, false);

    mFenceValues[mFrameIndex] += 1;
}
