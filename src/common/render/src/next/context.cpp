#include "stdafx.hpp"

#include "render/next/context.hpp"

namespace render = sm::render;

using CoreContext = render::next::CoreContext;
using CoreDevice = render::next::CoreDevice;
using ContextConfig = render::next::ContextConfig;

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

#pragma region Lifetime

void CoreContext::createDeviceState() {
    createAllocator();
    createDirectQueue();
}

#pragma region Constructor

CoreContext::CoreContext(ContextConfig config) noexcept(false)
    : mDebugFlags(config.flags)
    , mInstance(newInstance(config))
    , mDebugState(config.flags)
    , mDevice(selectDevice(config))
{
    createDeviceState();
    createSwapChain(config.swapChainFactory, config.swapChainInfo);
}

void CoreContext::setAdapter(AdapterLUID luid) {
    if (luid == mDevice.luid())
        return;

    FeatureLevel level = mDevice.level();

    CoreDevice newDevice = createDevice(luid, level, mDebugFlags);

    mSwapChain.reset();
    mDirectQueue.reset();
    mAllocator.reset();

    mDevice = std::move(newDevice);
    createDeviceState();
    createSwapChain(mSwapChainFactory, mSwapChainInfo);
}

void CoreContext::updateSwapChain(SurfaceInfo info) {

}
