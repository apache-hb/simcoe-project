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

HandleData::HandleData(std::string name, render::next::DeviceResource resource, D3D12_RESOURCE_STATES state)
    : mName(std::move(name))
    , mResource(std::move(resource))
    , mState(state)
{ }

RenderPass::RenderPass(RenderPassInfo info)
    : mInfo(std::move(info))
{ }
