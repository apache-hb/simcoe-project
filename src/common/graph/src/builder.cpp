#include "stdafx.hpp"

#include "graph/graph.hpp"

using namespace sm::render::next;
using namespace sm::graph;

///
/// handle builder
///

HandleUseBuilder::operator Handle() const {
    return mGraph.getHandle(mHandleUse);
}

D3D12_CPU_DESCRIPTOR_HANDLE HandleUseBuilder::createRenderTargetView() {
    HandleUseData& data = mGraph.getHandleUseData(mHandleUse);
    auto [index, host, device] = mGraph.createRenderTargetView(data.getHandle(), nullptr);
    data.rtvIndex = index;
    return host;
}

D3D12_CPU_DESCRIPTOR_HANDLE HandleUseBuilder::createRenderTargetView(D3D12_RENDER_TARGET_VIEW_DESC desc) {
    HandleUseData& data = mGraph.getHandleUseData(mHandleUse);
    auto [index, host, device] = mGraph.createRenderTargetView(data.getHandle(), &desc);
    data.rtvIndex = index;
    return host;
}

D3D12_CPU_DESCRIPTOR_HANDLE HandleUseBuilder::createDepthStencilView() {
    HandleUseData& data = mGraph.getHandleUseData(mHandleUse);
    auto [index, host, device] = mGraph.createDepthStencilView(data.getHandle(), nullptr);
    data.dsvIndex = index;
    return host;
}

D3D12_CPU_DESCRIPTOR_HANDLE HandleUseBuilder::createDepthStencilView(D3D12_DEPTH_STENCIL_VIEW_DESC desc) {
    HandleUseData& data = mGraph.getHandleUseData(mHandleUse);
    auto [index, host, device] = mGraph.createDepthStencilView(data.getHandle(), &desc);
    data.dsvIndex = index;
    return host;
}

D3D12_GPU_DESCRIPTOR_HANDLE HandleUseBuilder::createShaderResourceView() {
    HandleUseData& data = mGraph.getHandleUseData(mHandleUse);
    auto [index, host, device] = mGraph.createShaderResourceView(data.getHandle(), nullptr);
    data.srvIndex = index;
    return device;
}

D3D12_GPU_DESCRIPTOR_HANDLE HandleUseBuilder::createShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC desc) {
    HandleUseData& data = mGraph.getHandleUseData(mHandleUse);
    auto [index, host, device] = mGraph.createShaderResourceView(data.getHandle(), &desc);
    data.srvIndex = index;
    return device;
}

///
/// render pass builder
///

void RenderPassBuilder::addToGraph(ExecuteFn fn) {
    RenderPassInfo info {
        .name = mName,
        .type = mType,
        .execute = std::move(fn),
    };

    mGraph.addRenderPass(info);
}

HandleUseBuilder RenderPassBuilder::create(const std::string& name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc) {
    Handle handle = mGraph.newResourceHandle(name, state, desc);
    return write(handle, name, state);
}

HandleUseBuilder RenderPassBuilder::read(Handle handle, std::string name, D3D12_RESOURCE_STATES state) {
    return HandleUseBuilder { mGraph, mGraph.newResourceUsage(std::move(name), handle, state) };
}

HandleUseBuilder RenderPassBuilder::write(Handle handle, std::string name, D3D12_RESOURCE_STATES state) {
    return HandleUseBuilder { mGraph, mGraph.newResourceUsage(std::move(name), handle, state) };
}

Handle RenderGraphBuilder::getHandle(HandleUse handle) const {
    return mHandleUses[handle.mIndex].getHandle();
}

static DescriptorIndex fromPool(DescriptorPool& pool, size_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE host = pool.host(index);
    D3D12_GPU_DESCRIPTOR_HANDLE device = pool.isShaderVisible() ? pool.device(index) : D3D12_GPU_DESCRIPTOR_HANDLE{0};

    return DescriptorIndex { index, host, device };
}

DescriptorIndex RenderGraphBuilder::createRenderTargetView(Handle handle, const D3D12_RENDER_TARGET_VIEW_DESC *desc) {
    DescriptorPool& pool = mContext.getRtvHeap();
    ID3D12Device *device = mContext.getDevice();
    size_t index = pool.allocate();

    device->CreateRenderTargetView(mHandles[handle.mIndex].get(), desc, pool.host(index));

    return fromPool(pool, index);
}

DescriptorIndex RenderGraphBuilder::createDepthStencilView(Handle handle, const D3D12_DEPTH_STENCIL_VIEW_DESC *desc) {
    DescriptorPool& pool = mContext.getDsvHeap();
    ID3D12Device *device = mContext.getDevice();
    size_t index = pool.allocate();

    device->CreateDepthStencilView(mHandles[handle.mIndex].get(), desc, pool.host(index));

    return fromPool(pool, index);
}

DescriptorIndex RenderGraphBuilder::createShaderResourceView(Handle handle, const D3D12_SHADER_RESOURCE_VIEW_DESC *desc) {
    DescriptorPool& pool = mContext.getSrvHeap();
    ID3D12Device *device = mContext.getDevice();
    size_t index = pool.allocate();

    device->CreateShaderResourceView(mHandles[handle.mIndex].get(), desc, pool.host(index));

    return fromPool(pool, index);
}

///
/// rendergraph
///

static DeviceResource newDeviceResource(CoreContext& context, D3D12_RESOURCE_DESC desc, D3D12_RESOURCE_STATES state) {
    D3D12MA::ALLOCATION_DESC alloc = {
        .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
        .HeapType = D3D12_HEAP_TYPE_DEFAULT,
    };

    return DeviceResource{context, state, desc, alloc};
}

Handle RenderGraphBuilder::include(std::string name, ID3D12Resource *resource, D3D12_RESOURCE_STATES state) {
    HandleData data { std::move(name), resource, state };
    size_t index = mHandles.size();
    mHandles.emplace_back(std::move(data));
    return Handle { index };
}

void RenderGraphBuilder::addRenderPass(RenderPassInfo info) {
    mRenderPasses.emplace_back(std::move(info));
}

Handle RenderGraphBuilder::newResourceHandle(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc) {
    auto& resource = mDeviceResources.emplace_back(newDeviceResource(mContext, desc, state));
    return include(std::move(name), resource.get(), state);
}

HandleUse RenderGraphBuilder::newResourceUsage(std::string name, Handle handle, D3D12_RESOURCE_STATES state) {
    HandleUseData data { std::move(name), handle, state };
    size_t index = mHandleUses.size();
    mHandleUses.emplace_back(std::move(data));
    return HandleUse { index };
}

RenderPassBuilder RenderGraphBuilder::newRenderPass(std::string name, D3D12_COMMAND_LIST_TYPE type) {
    return RenderPassBuilder{*this, std::move(name), type};
}

RenderPassBuilder RenderGraphBuilder::graphics(std::string name) {
    return newRenderPass(std::move(name), D3D12_COMMAND_LIST_TYPE_DIRECT);
}

RenderPassBuilder RenderGraphBuilder::compute(std::string name) {
    return newRenderPass(std::move(name), D3D12_COMMAND_LIST_TYPE_COMPUTE);
}
