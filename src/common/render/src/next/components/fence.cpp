#include "stdafx.hpp"

#include "render/next/components.hpp"

using sm::render::next::CoreDevice;
using sm::render::next::Fence;

static ID3D12Fence *newFence(ID3D12Device1 *device, uint64_t value) {
    ID3D12Fence *fence;
    SM_THROW_HR(device->CreateFence(value, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    return fence;
}

Fence::GuardHandle Fence::newEvent(LPCSTR name) {
    HANDLE event = CreateEventA(nullptr, FALSE, FALSE, name);
    if (event == nullptr)
        sm::system::getLastError().raise();

    return GuardHandle{event, kCloseHandle};
}

Fence::Fence(CoreDevice& device, uint64_t initialValue) noexcept(false)
    : mFence(newFence(device.get(), initialValue))
    , mEvent(Fence::newEvent("FenceEvent"))
{ }

uint64_t Fence::value() const {
    return mFence->GetCompletedValue();
}

void Fence::wait(uint64_t value) {
    if (mFence->GetCompletedValue() < value) {
        SM_THROW_HR(mFence->SetEventOnCompletion(value, mEvent.get()));
        WaitForSingleObject(mEvent.get(), INFINITE);
    }
}
