#include "stdafx.hpp"

#include "render/next/components.hpp"

using sm::render::next::CoreDevice;
using sm::render::next::CommandBufferSet;

CommandBufferSet::CommandBufferSet(CoreDevice& device, D3D12_COMMAND_LIST_TYPE type, UINT frameCount, UINT first) noexcept(false)
    : mCurrentBuffer(first)
    , mAllocators(frameCount)
{
    for (UINT i = 0; i < frameCount; i++) {
        SM_THROW_HR(device->CreateCommandAllocator(type, IID_PPV_ARGS(&mAllocators[i])));
        mAllocators[i].rename(fmt::format("CommandAllocator[{}]", i));
    }

    SM_THROW_HR(device->CreateCommandList(0, type, currentAllocator(), nullptr, IID_PPV_ARGS(&mCommandList)));
    mCommandList.rename("CommandList");
}

void CommandBufferSet::reset(UINT index) {
    mCurrentBuffer = index;
    SM_THROW_HR(currentAllocator()->Reset());
    SM_THROW_HR(mCommandList->Reset(currentAllocator(), nullptr));
}

ID3D12GraphicsCommandList* CommandBufferSet::close() {
    SM_THROW_HR(mCommandList->Close());
    return mCommandList.get();
}
