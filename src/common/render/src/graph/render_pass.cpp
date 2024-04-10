#include "stdafx.hpp"

#include "render/graph/render_pass.hpp"

#include "render/graph.hpp"
#include "render/render.hpp"

using namespace sm;
using namespace sm::graph;

D3D12_GPU_VIRTUAL_ADDRESS RenderContext::gpu_address(Handle handle) const {
    return resource(handle)->GetGPUVirtualAddress();
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderContext::srv_host(AccessHandle handle) const {
    auto access = pass.get_access(handle);
    if (access.descriptor != UINT_MAX)
        return context.mSrvPool.cpu_handle((render::SrvIndex)access.descriptor);

    return context.mSrvPool.cpu_handle(graph.srv(access.index));
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::srv_device(AccessHandle handle) const {
    auto access = pass.get_access(handle);
    if (access.descriptor != UINT_MAX)
        return context.mSrvPool.gpu_handle((render::SrvIndex)access.descriptor);

    return context.mSrvPool.gpu_handle(graph.srv(access.index));
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderContext::uav_host(AccessHandle handle) const {
    auto access = pass.get_access(handle);
    if (access.descriptor != UINT_MAX)
        return context.mSrvPool.cpu_handle((render::SrvIndex)access.descriptor);

    return context.mSrvPool.cpu_handle(graph.uav(access.index));
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::uav_device(AccessHandle handle) const {
    auto access = pass.get_access(handle);
    if (access.descriptor != UINT_MAX)
        return context.mSrvPool.gpu_handle((render::SrvIndex)access.descriptor);

    return context.mSrvPool.gpu_handle(graph.uav(access.index));
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderContext::rtv_host(AccessHandle handle) const {
    auto access = pass.get_access(handle);
    if (access.descriptor != UINT_MAX)
        return context.mRtvPool.cpu_handle((render::RtvIndex)access.descriptor);

    return context.mRtvPool.cpu_handle(graph.rtv(access.index));
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::rtv_device(AccessHandle handle) const {
    auto access = pass.get_access(handle);
    if (access.descriptor != UINT_MAX)
        return context.mRtvPool.gpu_handle((render::RtvIndex)access.descriptor);

    return context.mRtvPool.gpu_handle(graph.rtv(access.index));
}

D3D12_CPU_DESCRIPTOR_HANDLE RenderContext::dsv_host(AccessHandle handle) const {
    auto access = pass.get_access(handle);
    if (access.descriptor != UINT_MAX)
        return context.mDsvPool.cpu_handle((render::DsvIndex)access.descriptor);

    return context.mDsvPool.cpu_handle(graph.dsv(access.index));
}

D3D12_GPU_DESCRIPTOR_HANDLE RenderContext::dsv_device(AccessHandle handle) const {
    auto access = pass.get_access(handle);
    if (access.descriptor != UINT_MAX)
        return context.mDsvPool.gpu_handle((render::DsvIndex)access.descriptor);

    return context.mDsvPool.gpu_handle(graph.dsv(access.index));
}

ID3D12Resource* RenderContext::resource(Handle handle) const {
    return graph.resource(handle);
}

const ResourceAccess& RenderPass::get_access(AccessHandle handle) const {
    switch (handle.type) {
    case AccessHandle::eRead:
        return reads[handle.index];
    case AccessHandle::eWrite:
        return writes[handle.index];
    case AccessHandle::eCreate:
        return creates[handle.index];
    }
}

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
