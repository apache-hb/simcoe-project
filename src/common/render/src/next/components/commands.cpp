#include "stdafx.hpp"

#include "render/next/components.hpp"

using sm::render::next::CoreDevice;
using sm::render::next::CommandBufferSet;

CommandBufferSet::CommandBufferSet(CoreDevice& device, D3D12_COMMAND_LIST_TYPE type, UINT frameCount) noexcept(false)
    : mCurrentBuffer(0)
    , mAllocators(frameCount)
{
    ID3D12Device* it = device.get();
    for (UINT i = 0; i < frameCount; i++) {
        SM_THROW_HR(it->CreateCommandAllocator(type, IID_PPV_ARGS(&mAllocators[i])));
    }

    SM_THROW_HR(it->CreateCommandList(0, type, currentAllocator(), nullptr, IID_PPV_ARGS(&mCommandList)));
    SM_THROW_HR(mCommandList->Close());
}

void CommandBufferSet::reset() {
    mCurrentBuffer = getNextIndex();
    SM_THROW_HR(currentAllocator()->Reset());
    SM_THROW_HR(mCommandList->Reset(currentAllocator(), nullptr));
}

ID3D12GraphicsCommandList* CommandBufferSet::close() {
    SM_THROW_HR(mCommandList->Close());
    return mCommandList.get();
}
