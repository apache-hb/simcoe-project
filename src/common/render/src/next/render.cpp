#include "stdafx.hpp"

#include "render/next/device.hpp"

namespace render = sm::render;

using CoreDevice = render::next::CoreDevice;
using RenderError = render::RenderError;

using render::Object;

static void onQueueMessage(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id, LPCSTR desc, void *user) {
    render::MessageCategory c{category};
    render::MessageSeverity s{severity};
    render::MessageID message{id};

    fmt::println(stderr, "{} {}: {}", c, message, desc);

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

void CoreDevice::setupCoreDevice(Adapter& adapter, FeatureLevel level) {
    SM_THROW_HR(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL(level), IID_PPV_ARGS(&mDevice)));
}

CoreDevice::CoreDevice(Adapter& adapter, FeatureLevel level, DebugFlags flags) noexcept(false)
    : mFeatureLevel(level)
    , mAdapter(std::addressof(adapter))
{
    bool enableInfoQueue = bool(flags & DebugFlags::eInfoQueue);

    setupCoreDevice(adapter, level);
    setupInfoQueue(enableInfoQueue);
}

#pragma region Public API

static constexpr D3D12MA::ALLOCATION_CALLBACKS kAllocCallbacks {
    .pAllocate = [](size_t size, size_t alignment, void *user) -> void * {
        return _aligned_malloc(size, alignment);
    },
    .pFree = [](void *ptr, void *user) {
        _aligned_free(ptr);
    },
};

Object<D3D12MA::Allocator> CoreDevice::newAllocator(D3D12MA::ALLOCATOR_FLAGS flags) noexcept(false) {
    D3D12MA::ALLOCATOR_DESC desc {
        .Flags = flags,
        .pDevice = mDevice.get(),
        .pAllocationCallbacks = &kAllocCallbacks,
        .pAdapter = mAdapter->get(),
    };

    Object<D3D12MA::Allocator> allocator;
    SM_THROW_HR(D3D12MA::CreateAllocator(&desc, &allocator));

    return allocator;
}

Object<ID3D12CommandQueue> CoreDevice::newCommandQueue(D3D12_COMMAND_QUEUE_DESC desc) noexcept(false) {
    Object<ID3D12CommandQueue> queue;
    SM_THROW_HR(mDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));

    return queue;
}

void CoreDevice::reset() noexcept {
    mInfoQueue.reset();
    mDevice.reset();
}

bool CoreDevice::setDeviceRemoved() noexcept {
    Object<ID3D12Device5> device;
    if (RenderError error = mDevice.query(&device)) {
        LOG_WARN(GpuLog, "failed to get ID3D12Device5: {}", error);
        return false;
    }

    device->RemoveDevice();
    return true;
}

bool CoreDevice::isDeviceRemoved() const noexcept {
    return mDevice->GetDeviceRemovedReason() != S_OK;
}
