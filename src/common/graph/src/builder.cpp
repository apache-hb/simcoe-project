#include "stdafx.hpp"

#include "graph/graph.hpp"

using namespace sm::render::next;
using namespace sm::graph;

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

HandleBuilder RenderPassBuilder::create(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc) {
    return HandleBuilder { mGraph, mGraph.newResourceHandle(std::move(name), state, desc) };
}

HandleUseBuilder RenderPassBuilder::read(Handle handle, std::string name, D3D12_RESOURCE_STATES state) {
    return HandleUseBuilder { mGraph, mGraph.newResourceUsage(std::move(name), handle, state) };
}

HandleUseBuilder RenderPassBuilder::write(Handle handle, std::string name, D3D12_RESOURCE_STATES state) {
    return HandleUseBuilder { mGraph, mGraph.newResourceUsage(std::move(name), handle, state) };
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

Handle RenderGraphBuilder::include(ID3D12Resource *resource, D3D12_RESOURCE_STATES state) {
    return Handle { 0 };
}

void RenderGraphBuilder::addRenderPass(RenderPassInfo info) {
    mRenderPasses.emplace_back(std::move(info));
}

Handle RenderGraphBuilder::newResourceHandle(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc) {
    HandleData data { std::move(name), newDeviceResource(mContext, desc, state), state };
    size_t index = mHandles.size();
    mHandles.emplace_back(std::move(data));
    return Handle { index };
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
