#include "stdafx.hpp"

#include "render/graph/events.hpp"
#include "render/graph/render_pass.hpp"

#include "render/graph.hpp"

using namespace sm;
using namespace sm::graph;

void events::ResourceBarrier::build(FrameGraph& graph) {
    barriers.clear();

    for (const Transition& transition : transitions) {
        ID3D12Resource *resource = graph.resource(transition.resource);

        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, transition.before, transition.after));

#if SMC_RENDER_FRAMEGRAPH_TRACE
        auto name = render::getObjectDebugName(resource);
        LOG_INFO(RenderLog, " - {} (before: {}, after: {})", name, transition.before, transition.after);
#endif
    }

    for (const UnorderedAccess& uav : uavs) {
        ID3D12Resource *resource = graph.resource(uav.resource);

        barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(resource));

#if SMC_RENDER_FRAMEGRAPH_TRACE
        auto name = render::getObjectDebugName(resource);
        LOG_INFO(RenderLog, " - {} (UAV)", name);
#endif
    }
}

bool RenderPass::uses_handle(Handle handle) const {
    return find(eRead | eWrite, [&](const ResourceAccess& access) {
        return access.index == handle;
    });
}

bool RenderPass::updates_handle(Handle handle) const {
    return find(eWrite | eRead | eCreate, [&](const ResourceAccess& access) {
        return access.index == handle;
    });
}

Usage RenderPass::get_handle_usage(Handle handle) const {
    Usage result = Usage::eUnknown;

    foreach(eRead | eWrite, [&](const ResourceAccess& access) {
        if (access.index == handle) {
            result = access.usage;
        }
    });

    return result;
}

bool RenderPass::depends_on(const RenderPass& other) const {
    return find(eRead, [&](const ResourceAccess& access) {
        return other.updates_handle(access.index);
    });
}

ResourceAccess RenderPass::getHandleAccess(Handle handle) const {
    ResourceAccess result;

    foreach(eRead | eWrite, [&](const ResourceAccess& access) {
        if (access.index == handle) {
            result = access;
        }
    });

    return result;
}

D3D12_RESOURCE_STATES ResourceAccess::getStateFlags() const {
    D3D12_RESOURCE_STATES usageState = getStateFromUsage(usage);
    return states | usageState;
}

bool ResourceAccess::isReadState() const {
    return isReadOnlyState(getStateFlags());
}
