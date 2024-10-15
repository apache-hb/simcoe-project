#include "stdafx.hpp"

#include "render/render.hpp"

namespace render = sm::render;

using CoreDevice = render::next::CoreDevice;
using RenderError = render::RenderError;

using render::Object;

void CoreDevice::setupCoreDevice(Adapter& adapter, FeatureLevel level) {
    SM_THROW_HR(D3D12CreateDevice(adapter.get(), level.as_facade(), IID_PPV_ARGS(&mDevice)));
}

static void onQueueMessage(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR desc, void *user) {
    render::MessageCategory c{category};
    render::MessageSeverity s{severity};
    render::MessageID message{id};

    switch (severity) {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
        LOG_PANIC(GpuLog, "{} {}: {}", c, message, desc);
        break;
    case D3D12_MESSAGE_SEVERITY_ERROR:
        LOG_ERROR(GpuLog, "{} {}: {}", c, message, desc);
        break;
    case D3D12_MESSAGE_SEVERITY_WARNING:
        LOG_WARN(GpuLog, "{} {}: {}", c, message, desc);
        break;
    case D3D12_MESSAGE_SEVERITY_INFO:
        LOG_INFO(GpuLog, "{} {}: {}", c, message, desc);
        break;
    case D3D12_MESSAGE_SEVERITY_MESSAGE:
        LOG_DEBUG(GpuLog, "{} {}: {}", c, message, desc);
        break;
    }
}

void CoreDevice::setupInfoQueue(bool enabled) {
    if (!enabled)
        return;

    if (RenderError error = mDevice.query(&mInfoQueue)) {
        LOG_WARN(GpuLog, "failed to get ID3D12InfoQueue1: {}", error);
        return;
    }

    SM_THROW_HR(mInfoQueue->RegisterMessageCallback(&onQueueMessage, D3D12_MESSAGE_CALLBACK_FLAG_NONE, this, &mCookie));
    SM_THROW_HR(mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
    SM_THROW_HR(mInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));

    LOG_INFO(GpuLog, "info queue enabled with cookie {}", mCookie);
}

CoreDevice::CoreDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags) noexcept(false)
    : mFeatureLevel(level)
    , mAdapterLUID(adapter.luid())
{
    bool enableInfoQueue = flags.test(DebugFlags::eInfoQueue);

    setupCoreDevice(adapter, level);
    setupInfoQueue(enableInfoQueue);
}
