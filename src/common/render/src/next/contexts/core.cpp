#include "stdafx.hpp"

#include "render/next/context/core.hpp"

namespace render = sm::render;

using DeviceContext = render::next::DeviceContext;
using CoreDevice = render::next::CoreDevice;
using DeviceContextConfig = render::next::DeviceContextConfig;

using render::RenderResult;
using render::RenderError;

#pragma region Instance

static render::Instance newInstance(const DeviceContextConfig& ctxConfig) {
    render::InstanceConfig config {
        .flags = ctxConfig.flags,
        .preference = ctxConfig.autoSearchBehaviour,
    };

    return render::Instance{config};
}

#pragma region Device

CoreDevice DeviceContext::createDevice(AdapterLUID luid, FeatureLevel level, DebugFlags flags) {
    if (Adapter *adapter = mInstance.findAdapterByLUID(luid)) {
        return CoreDevice{*adapter, level, flags};
    }

    throw RenderException{E_FAIL, fmt::format("Adapter {} not found.", luid)};
}

RenderResult<CoreDevice> DeviceContext::createDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags) {
    try {
        CoreDevice device{adapter, level, flags};
        return std::move(device);
    } catch (RenderException& e) {
        return std::unexpected(RenderError{E_FAIL, "Failed to create device with acceptable feature level"});
    }
}

CoreDevice DeviceContext::selectAutoDevice(DeviceSearchOptions options) {
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

CoreDevice DeviceContext::selectPreferredDevice(AdapterLUID luid, DeviceSearchOptions options) {
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

CoreDevice DeviceContext::selectDevice(const DeviceContextConfig& config) {
    DeviceSearchOptions options {
        .level = config.targetLevel,
        .flags = config.flags,
        .allowSoftwareAdapter = config.allowSoftwareAdapter,
    };

    return selectPreferredDevice(config.adapterOverride, options);
}

void DeviceContext::resetDeviceResources() {
    for (IDeviceBound *resource : mDeviceBoundResources) {
        resource->resetDeviceResources();
    }

    mAllocator.reset();
    mDevice.reset();

    mInstance.reportLiveObjects();
}

void DeviceContext::recreateCurrentDevice() {
    AdapterLUID luid = mDevice.luid();
    FeatureLevel level = mDevice.level();

    // when recreating the device, we reset all resources first
    // since we will be using the same device. Preserving any resource references
    // will cause d3d12 to give us the same (still removed) device handle.
    resetDeviceResources();
    mDevice = createDevice(luid, level, mDebugFlags);
}

void DeviceContext::moveToNewDevice(AdapterLUID luid) {
    // when moving to a new device, we create the device first
    // and then reset all resources.
    // this is to ensure we don't leave ourselves in an invalid state
    // if the new device creation fails.
    FeatureLevel level = mDevice.level();
    CoreDevice newDevice = createDevice(luid, level, mDebugFlags);

    resetDeviceResources();

    mDevice = std::move(newDevice);
}

#pragma region Public API

static constexpr D3D12MA::ALLOCATOR_FLAGS kAllocatorFlags
    = D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED
    | D3D12MA::ALLOCATOR_FLAG_SINGLETHREADED;

DeviceContext::DeviceContext(DeviceContextConfig config) noexcept(false)
    : mDebugFlags(config.flags)
    , mInstance(newInstance(config))
    , mDebugState(config.flags)
    , mDevice(selectDevice(config))
    , mAllocator(mDevice.newAllocator(kAllocatorFlags))
{ }

void DeviceContext::setAdapter(AdapterLUID luid) {
    if (luid == mDevice.luid()) {
        recreateCurrentDevice();
    } else {
        moveToNewDevice(luid);
    }
}

void DeviceContext::setDeviceRemoved() {
    mDevice.setDeviceRemoved();
}
