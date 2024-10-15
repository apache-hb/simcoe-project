#include "stdafx.hpp"

#include "render/render.hpp"

using CoreDebugState = sm::render::next::CoreDebugState;

using sm::render::Object;
using sm::render::RenderError;

static bool enableGpuValidation(Object<ID3D12Debug1>& debug, bool enabled) {
    if (!enabled)
        return false;

    Object<ID3D12Debug3> debug3;
    if (RenderError hr = debug.query(&debug3)) {
        LOG_WARN(GpuLog, "failed to get ID3D12Debug3: {}", hr);
        return false;
    }

    debug3->SetEnableGPUBasedValidation(true);
    return true;
}

static bool enableAutoName(Object<ID3D12Debug1>& debug, bool enabled) {
    if (!enabled)
        return false;

    Object<ID3D12Debug5> debug5;
    if (RenderError hr = debug.query(&debug5)) {
        LOG_WARN(GpuLog, "failed to get ID3D12Debug5: {}", hr);
        return false;
    }

    debug5->SetEnableAutoName(true);
    return true;
}

static bool setupDeviceRemovedInfo(bool enabled) {
    if (!enabled)
        return false;

    Object<ID3D12DeviceRemovedExtendedDataSettings> dred;
    if (RenderError hr = D3D12GetDebugInterface(IID_PPV_ARGS(&dred))) {
        LOG_WARN(GpuLog, "failed to get ID3D12DeviceRemovedExtendedDataSettings: {}", hr);
        return false;
    }

    dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

    LOG_INFO(GpuLog, "enabled device removed info");
    return true;
}

bool CoreDebugState::setupDebugLayer(bool enabled) {
    if (!enabled)
        return false;

    if (RenderError error = D3D12GetDebugInterface(IID_PPV_ARGS(&mDebug))) {
        LOG_WARN(GpuLog, "failed to get ID3D12Debug1: {}", error);
        return false;
    }

    mDebug->EnableDebugLayer();
    LOG_INFO(GpuLog, "enabled debug layer");
    return true;
}

CoreDebugState::CoreDebugState(DebugFlags flags) noexcept(false) {
    bool gpuValidation = flags.test(DebugFlags::eGpuValidation);
    bool nameObjects = flags.test(DebugFlags::eAutoName);
    bool deviceRemovedInfo = flags.test(DebugFlags::eDeviceRemovedInfo);

    bool enableDebugLayer = gpuValidation || nameObjects || deviceRemovedInfo;

    mDebugLayer = setupDebugLayer(enableDebugLayer);
    mGpuValidation = enableGpuValidation(mDebug, gpuValidation);
    mAutoNamedObjects = enableAutoName(mDebug, nameObjects);
    mDeviceRemovedInfo = setupDeviceRemovedInfo(deviceRemovedInfo);
}
