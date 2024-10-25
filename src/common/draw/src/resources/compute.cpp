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
    mComputeCommandSet = std::make_unique<CommandBufferSet>(device, D3D12_COMMAND_LIST_TYPE_COMPUTE, length);
}

void ComputeContext::createFence() {
    CoreDevice& device = mContext.getCoreDevice();
    mComputeFence = std::make_unique<Fence>(device, 0, "ComputeFence");
}

void ComputeContext::setup(UINT length) {
    createCommandQueue();
    createCommandList(length);
    createFence();
}

void ComputeContext::reset() noexcept {
    mComputeCommandSet.reset();
    mComputeQueue.reset();
    mComputeFence.reset();
}

void ComputeContext::create() {
    setup(mContext.getSwapChainLength());
}

void ComputeContext::update(render::next::SurfaceInfo info) {
    createCommandList(info.length);
}
