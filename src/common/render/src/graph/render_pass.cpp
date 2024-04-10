#include "stdafx.hpp"

#include "render/graph/render_pass.hpp"

using namespace sm;
using namespace sm::graph;

bool RenderPass::uses_handle(Handle handle) const {
    return find(eRead | eWrite, [&](const ResourceAccess& access) {
        return access.index == handle;
    });
}

bool RenderPass::updates_handle(Handle handle) const {
    return find(eWrite | eCreate, [&](const ResourceAccess& access) {
        return access.index == handle;
    });
}

Usage RenderPass::get_handle_usage(Handle handle) {
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
