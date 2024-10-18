#include "stdafx.hpp"

#include "render/next/context/core.hpp"

namespace render = sm::render;

using CoreContext = render::next::CoreContext;
using CoreDevice = render::next::CoreDevice;
using ContextConfig = render::next::ContextConfig;

using sm::render::next::SurfaceInfo;

using render::RenderResult;
using render::RenderError;

#pragma region Instance

static render::Instance newInstance(const ContextConfig& ctxConfig) {
    render::InstanceConfig config {
        .flags = ctxConfig.flags,
        .preference = ctxConfig.autoSearchBehaviour,
    };

    return render::Instance{config};
}

UINT CoreContext::getRequiredRtvHeapSize() const {
    UINT maxSwapChainLength = mSwapChainFactory->maxSwapChainLength();
    return maxSwapChainLength + mExtraRtvHeapSize;
}

#pragma region Device

CoreDevice CoreContext::createDevice(AdapterLUID luid, FeatureLevel level, DebugFlags flags) {
    if (Adapter *adapter = mInstance.findAdapterByLUID(luid)) {
        return CoreDevice{*adapter, level, flags};
    }

    throw RenderException{E_FAIL, fmt::format("Adapter {} not found.", luid)};
}

RenderResult<CoreDevice> CoreContext::createDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags) {
    try {
        CoreDevice device{adapter, level, flags};
        return std::move(device);
    } catch (RenderException& e) {
        return std::unexpected(RenderError{E_FAIL, "Failed to create device with acceptable feature level"});
    }
}

CoreDevice CoreContext::selectAutoDevice(DeviceSearchOptions options) {
    const auto& [level, flags, allowSoftwareAdapter] = options;

    bool warpAdapter = bool(flags & DebugFlags::eWarpAdapter);
    if (warpAdapter) {
        LOG_INFO(RenderLog, "WARP adapter specified.");
        return CoreDevice{mInstance.getWarpAdapter(), level, flags};
    }

    LOG_INFO(RenderLog, "Selecting device automatically, preferring {} devices.", mInstance.preference());

    for (Adapter& adapter : mInstance.adapters()) {
        RenderResult<CoreDevice> result = createDevice(adapter, level, flags);
        if (result) {
            LOG_INFO(RenderLog, "Created device on adapter {}.", adapter.name());
            return std::move(result.value());
        }
    }

    if (allowSoftwareAdapter) {
        LOG_INFO(RenderLog, "No suitable hardware adapter found, falling back to software adapter.");
        return CoreDevice{mInstance.getWarpAdapter(), level, flags};
    }

    LOG_ERROR(RenderLog, "No suitable hardware adapter found.");
    throw RenderException{E_FAIL, fmt::format("No hardware adapter found with support for {}.", level)};
}

CoreDevice CoreContext::selectPreferredDevice(AdapterLUID luid, DeviceSearchOptions options) {
    if (!luid.isPresent()) {
        LOG_INFO(RenderLog, "No adapter LUID override, using automatic selection.");
        return selectAutoDevice(options);
    }

    Adapter *adapter = mInstance.findAdapterByLUID(luid);
    if (adapter == nullptr) {
        LOG_WARN(RenderLog, "Adapter {} is not present, falling back to automatic selection.", luid);
        return selectAutoDevice(options);
    }

    LOG_INFO(RenderLog, "Adapter {} found, creating device.", luid);
    RenderResult<CoreDevice> result = createDevice(*adapter, options.level, options.flags);
    if (result)
        return std::move(result.value());

    LOG_WARN(RenderLog, "Failed to create device with feature level {}, falling back to automatic selection.", options.level);

    return selectAutoDevice(options);
}

CoreDevice CoreContext::selectDevice(const ContextConfig& config) {
    DeviceSearchOptions options {
        .level = config.targetLevel,
        .flags = config.flags,
        .allowSoftwareAdapter = config.allowSoftwareAdapter,
    };

    return selectPreferredDevice(config.adapterOverride, options);
}

void CoreContext::resetDeviceResources() {
    mPresentFence.reset();
    mBackBuffers.clear();
    mSwapChain.reset();
    mDirectQueue.reset();
    mCommandBufferSet.reset();
    mRtvHeap.reset();
    mAllocator.reset();
    mDevice.reset();

    mInstance.reportLiveObjects();
}

void CoreContext::recreateCurrentDevice() {
    AdapterLUID luid = mDevice.luid();
    FeatureLevel level = mDevice.level();

    // when recreating the device, we reset all resources first
    // as we will be using the same device and preserving the resources
    // will cause d3d12 to give us the same (still removed) device handle.
    resetDeviceResources();
    mDevice = createDevice(luid, level, mDebugFlags);
}

void CoreContext::moveToNewDevice(AdapterLUID luid) {
    // when moving to a new device, we create the device first
    // and then reset all resources.
    // this is to ensure we don't leave ourselves in an invalid state
    // if the new device creation fails.
    FeatureLevel level = mDevice.level();
    CoreDevice device = createDevice(luid, level, mDebugFlags);

    resetDeviceResources();
    mDevice = std::move(device);
}

#pragma region RTV Pool

void CoreContext::createRtvHeap() {
    UINT size = getRequiredRtvHeapSize();
    mRtvHeap = std::make_unique<DescriptorPool>(mDevice, size, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, false);
}

#pragma region Allocator

static constexpr D3D12MA::ALLOCATOR_FLAGS kAllocatorFlags
    = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED
    | D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;

void CoreContext::createAllocator() {
    mAllocator = mDevice.newAllocator(kAllocatorFlags);
}

#pragma region Direct Queue

void CoreContext::createDirectQueue() {
    mDirectQueue = mDevice.newCommandQueue({ .Type = D3D12_COMMAND_LIST_TYPE_DIRECT });
}

#pragma region Direct Command List

void CoreContext::createDirectCommandList() {
    mCommandBufferSet = std::make_unique<CommandBufferSet>(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT, mBackBuffers.size());
}

#pragma region SwapChain

void CoreContext::createSwapChain(ISwapChainFactory *factory, SurfaceInfo info) {
    CTASSERT(factory != nullptr);

    mSwapChainFactory = factory;
    SurfaceCreateObjects objects {
        .instance = mInstance,
        .device = mDevice,
        .queue = mDirectQueue,
    };

    mSwapChain = std::unique_ptr<ISwapChain>(mSwapChainFactory->newSwapChain(objects, info));
    CTASSERT(mSwapChain != nullptr);

    mSwapChainInfo = info;
}

CoreContext::BackBufferList CoreContext::getSwapChainSurfaces(uint64_t initialValue) const {
    UINT length = mSwapChain->length();

    BackBufferList buffers{length};
    for (UINT i = 0; i < length; i++) {
        Object<ID3D12Resource> surface = mSwapChain->getSurface(i);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->getHostDescriptorHandle(i);
        mDevice->CreateRenderTargetView(surface.get(), nullptr, rtvHandle);

        buffers[i] = BackBuffer {
            .surface = std::move(surface),
            .rtvHandle = rtvHandle,
            .value = initialValue,
        };
    }

    return buffers;
}

void CoreContext::createBackBuffers(uint64_t initialValue) {
    mBackBuffers = getSwapChainSurfaces(initialValue);
    setFrameIndex(mSwapChain->currentSurfaceIndex());
}

uint64_t& CoreContext::fenceValueAt(UINT index) {
    return mBackBuffers[index].value;
}

D3D12_CPU_DESCRIPTOR_HANDLE CoreContext::rtvHandleAt(UINT index) {
    return mBackBuffers[index].rtvHandle;
}

ID3D12Resource *CoreContext::surfaceAt(UINT index) {
    return mBackBuffers[index].surface.get();
}

#pragma region Present Fence

void CoreContext::createPresentFence() {
    mPresentFence = std::make_unique<Fence>(mDevice);
    fenceValueAt(mCurrentBackBuffer) += 1;
}

#pragma region Lifetime

void CoreContext::createDeviceState() {
    createAllocator();
    createDirectQueue();
}

#pragma region Device Timeline

void CoreContext::advanceFrame() {
    uint64_t current = fenceValueAt(mCurrentBackBuffer);
    SM_THROW_HR(mDirectQueue->Signal(mPresentFence->get(), current));

    setFrameIndex(mSwapChain->currentSurfaceIndex());

    uint64_t value = fenceValueAt(mCurrentBackBuffer);
    mPresentFence->wait(value);

    fenceValueAt(mCurrentBackBuffer) = current + 1;
}

void CoreContext::flushDevice() {
    uint64_t current = fenceValueAt(mCurrentBackBuffer)++;
    SM_THROW_HR(mDirectQueue->Signal(mPresentFence->get(), current));

    mPresentFence->wait(current);
}

void CoreContext::setFrameIndex(UINT index) {
    mCurrentBackBuffer = index;
    mAllocator->SetCurrentFrameIndex(index);
}

#pragma region Constructor

CoreContext::CoreContext(ContextConfig config) noexcept(false)
    : mExtraRtvHeapSize(config.rtvHeapSize)
    , mDebugFlags(config.flags)
    , mInstance(newInstance(config))
    , mDebugState(config.flags)
    , mDevice(selectDevice(config))
{
    createDeviceState();
    createSwapChain(config.swapChainFactory, config.swapChainInfo);
    createRtvHeap();
    createBackBuffers(0);
    createDirectCommandList();
    createPresentFence();
}

CoreContext::~CoreContext() noexcept try {
    flushDevice();
} catch (const render::RenderException& e) {
    LOG_ERROR(RenderLog, "Failed to flush device: {}", e);
} catch (...) {
    LOG_ERROR(RenderLog, "Failed to flush device: unknown error");
}

void CoreContext::setAdapter(AdapterLUID luid) {
    try {
        flushDevice();
    } catch (const render::RenderException& e) {
        LOG_WARN(GpuLog, "Flushing device during adapter change failed: {}. This is OK when recovering from a device removal.", e);
    }

    if (luid == mDevice.luid()) {
        recreateCurrentDevice();
    } else {
        moveToNewDevice(luid);
    }

    createDeviceState();
    createSwapChain(mSwapChainFactory, mSwapChainInfo);
    createRtvHeap();
    createBackBuffers(0);
    createDirectCommandList();
    createPresentFence();
}

void CoreContext::updateSwapChain(SurfaceInfo info) {
    flushDevice();

    mBackBuffers.clear();
    mSwapChain->updateSurfaceInfo(info);
    mSwapChainInfo = info;

    createBackBuffers(fenceValueAt(mCurrentBackBuffer));
    createDirectCommandList();
}

void CoreContext::present() {
    CommandBufferSet& commands = *mCommandBufferSet;

    ID3D12Resource *surface = surfaceAt(mCurrentBackBuffer);
    const D3D12_RESOURCE_BARRIER cIntoRenderTarget[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            surface,
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET
        )
    };

    const D3D12_RESOURCE_BARRIER cIntoPresent[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            surface,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT
        )
    };

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = { rtvHandleAt(mCurrentBackBuffer) };

    commands->ResourceBarrier(_countof(cIntoRenderTarget), cIntoRenderTarget);

    commands->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    commands->ClearRenderTargetView(rtvHandle, mSwapChainInfo.clearColour.data(), 0, nullptr);

    commands->ResourceBarrier(_countof(cIntoPresent), cIntoPresent);

    ID3D12CommandList *lists[] = { commands.close() };
    mDirectQueue->ExecuteCommandLists(_countof(lists), lists);

    mSwapChain->present(0);

    advanceFrame();

    mCommandBufferSet->reset(mCurrentBackBuffer);
}

bool CoreContext::removeDevice() noexcept {
    return mDevice.setDeviceRemoved();
}
