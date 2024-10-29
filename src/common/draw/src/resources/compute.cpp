#include "draw/resources/compute.hpp"

using namespace sm;
using namespace sm::render::next;
using namespace sm::draw::next;

ComputeContext::ComputeContext(render::next::CoreContext& ctx)
    : Super(ctx)
{ }

void ComputeContext::createCommandQueue() {
    CoreDevice& device = mContext.getCoreDevice();
    mComputeQueue = device.newCommandQueue({
        .Type = D3D12_COMMAND_LIST_TYPE_COMPUTE,
    });
}

void ComputeContext::createCommandList(UINT length) {
    CoreDevice& device = mContext.getCoreDevice();
    mCommandSet = std::make_unique<CommandBufferSet>(device, D3D12_COMMAND_LIST_TYPE_COMPUTE, length);
    mCommandSet->close();
}

void ComputeContext::createFence() {
    CoreDevice& device = mContext.getCoreDevice();
    mFenceValue = 0;
    SM_THROW_HR(device->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mComputeFence)));
}

void ComputeContext::setup(UINT length) {
    createCommandQueue();
    createCommandList(length);
    createFence();
}

void ComputeContext::reset() noexcept {
    mComputeQueue->Signal(mComputeFence.get(), ++mFenceValue);
    mComputeFence->SetEventOnCompletion(mFenceValue, nullptr);

    mCommandSet.reset();
    mComputeQueue.reset();
    mComputeFence.reset();
}

void ComputeContext::create() {
    setup(mContext.getSwapChainLength());
}

void ComputeContext::update(render::next::SurfaceInfo info) {
    createCommandList(info.length);
}

void ComputeContext::begin(UINT index) {
    mCommandSet->reset(index);
}

void ComputeContext::end() {
    ID3D12CommandList *lists[] = { mCommandSet->close() };
    mComputeQueue->ExecuteCommandLists(_countof(lists), lists);
}

void ComputeContext::blockQueueUntil(ID3D12CommandQueue *queue) {
    syncDeviceTimeline(mComputeQueue.get(), queue, mComputeFence.get(), ++mFenceValue);
}

void ComputeContext::waitOnQueue(ID3D12CommandQueue *queue) {
    syncDeviceTimeline(queue, mComputeQueue.get(), mComputeFence.get(), ++mFenceValue);
}
