#include "stdafx.hpp"

#include "graph/graph.hpp"

using namespace sm;
using namespace sm::graph;

///
/// handles
///

HandleData::HandleData(std::string name, ID3D12Resource *resource, D3D12_RESOURCE_STATES state)
    : mName(std::move(name))
    , mResource(resource)
    , mState(state)
{ }

HandleUseData::HandleUseData(std::string name, Handle handle, D3D12_RESOURCE_STATES state)
    : mName(std::move(name))
    , mHandle(handle)
    , mState(state)
{ }

RenderPass::RenderPass(RenderPassInfo info)
    : mInfo(std::move(info))
{ }
