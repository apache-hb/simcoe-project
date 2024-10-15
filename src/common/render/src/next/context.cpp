#include "stdafx.hpp"

#include "render/render.hpp"

namespace render = sm::render;

using CoreContext = render::next::CoreContext;
using CoreDevice = render::next::CoreDevice;
using ContextConfig = render::next::ContextConfig;

using render::RenderResult;

static render::Instance newInstance(const ContextConfig& ctxConfig) {
    render::InstanceConfig config {
        .flags = ctxConfig.flags,
        .preference = ctxConfig.autoSearchBehaviour,
    };

    return render::Instance{config};
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

    bool warpAdapter = flags.test(DebugFlags::eWarpAdapter);
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

CoreContext::CoreContext(ContextConfig config) noexcept(false)
    : mInstance(newInstance(config))
    , mDebugState(config.flags)
    , mDevice(selectDevice(config))
{

}
