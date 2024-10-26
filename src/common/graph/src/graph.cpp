#include "stdafx.hpp"

#include "graph/graph.hpp"

using namespace sm;
using namespace sm::graph;

///
/// handles
///

Handle::Handle(size_t index)
    : mIndex(index)
{ }

HandleData::HandleData(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc)
    : mName(std::move(name))
    , mState(state)
    , mDesc(desc)
{ }

RenderPass::RenderPass(RenderPassInfo)
{ }

///
/// render pass builder
///

void RenderPassBuilder::addToGraph(ExecuteFn fn) {
    RenderPassInfo info {
        .name = mName,
        .type = mType,
        .create = std::move(mCreate),
        .read = std::move(mRead),
        .write = std::move(mWrite),
        .execute = std::move(fn),
    };

    mGraph.addRenderPass(info);
}

Handle RenderPassBuilder::create(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc) {
    Handle handle = mGraph.newResourceHandle(std::move(name), state, desc);

    mCreate.emplace_back(HandleCreateInfo {
        .handle = handle,
    });

    return handle;
}

void RenderPassBuilder::read(Handle handle, std::string name, D3D12_RESOURCE_STATES state) {
    mRead.emplace_back(HandleReadInfo {
        .handle = handle,
        .name = std::move(name),
        .state = state,
    });
}

void RenderPassBuilder::write(Handle handle, std::string name, D3D12_RESOURCE_STATES state) {
    mWrite.emplace_back(HandleWriteInfo {
        .handle = handle,
        .name = std::move(name),
        .state = state,
    });
}

///
/// rendergraph
///

Handle RenderGraph::include(ID3D12Resource *resource, D3D12_RESOURCE_STATES state) {
    return Handle { 0 };
}

void RenderGraph::addRenderPass(RenderPassInfo info) {
    mRenderPasses.emplace_back(std::move(info));
}

Handle RenderGraph::newResourceHandle(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc) {
    HandleData data{std::move(name), state, desc};
    size_t index = mHandles.size();
    mHandles.emplace_back(std::move(data));
    return Handle { index };
}

RenderPassBuilder RenderGraph::newRenderPass(std::string name, D3D12_COMMAND_LIST_TYPE type) {
    return RenderPassBuilder{*this, std::move(name), type};
}

RenderPassBuilder RenderGraph::graphics(std::string name) {
    return newRenderPass(std::move(name), D3D12_COMMAND_LIST_TYPE_DIRECT);
}

RenderPassBuilder RenderGraph::compute(std::string name) {
    return newRenderPass(std::move(name), D3D12_COMMAND_LIST_TYPE_COMPUTE);
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::rtv(Handle handle) {
    return D3D12_CPU_DESCRIPTOR_HANDLE {};
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderGraph::dsv(Handle handle) {
    return D3D12_CPU_DESCRIPTOR_HANDLE {};
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderGraph::srv(Handle handle) {
    return D3D12_GPU_DESCRIPTOR_HANDLE {};
}
