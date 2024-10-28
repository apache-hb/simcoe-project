#include "stdafx.hpp"

#include "render/next/components.hpp"

using sm::render::next::CoreDevice;
using sm::render::next::Fence;

static ID3D12Fence *newFence(CoreDevice& device, uint64_t value) {
    ID3D12Fence *fence;
    SM_THROW_HR(device->CreateFence(value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    return fence;
}

wil::unique_handle Fence::newEvent(LPCSTR name) {
    HANDLE event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (event == nullptr)
        throw OsException{GetLastError()};

    return wil::unique_handle{event};
}

Fence::Fence(CoreDevice& device, uint64_t initialValue, const char *name) noexcept(false)
    : mFence(newFence(device, initialValue))
    , mEvent(Fence::newEvent(name))
{
    mFence.rename(name);
}

uint64_t Fence::completedValue() const {
    return mFence->GetCompletedValue();
}

void Fence::waitForCompetedValue(uint64_t value) {
    if (completedValue() < value) {
        waitForEvent(value);
    }
}

void Fence::waitForEvent(uint64_t value) {
    SM_THROW_HR(mFence->SetEventOnCompletion(value, mEvent.get()));
    DWORD dwResult = WaitForSingleObject(mEvent.get(), INFINITE);
    if (dwResult != WAIT_OBJECT_0) {
        throw OsException{GetLastError()};
    }
}

void Fence::signal(ID3D12CommandQueue *queue, uint64_t value) {
    SM_THROW_HR(queue->Signal(mFence.get(), value));
}
