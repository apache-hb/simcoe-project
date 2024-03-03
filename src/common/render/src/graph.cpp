#include "render/graph.hpp"

#include "core/map.hpp"
#include "core/stack.hpp"

#include "d3dx12_core.h"
#include "d3dx12_barriers.h"

using namespace sm;
using namespace sm::graph;

using enum render::ResourceState::Inner;

using PassBuilder = FrameGraph::PassBuilder;

void PassBuilder::read(Handle handle, Access access) {
    mRenderPass.reads.push_back({handle, access});
}

void PassBuilder::write(Handle handle, Access access) {
    mRenderPass.writes.push_back({handle, access});
    if (mFrameGraph.is_imported(handle)) {
        mRenderPass.has_side_effects = true;
    }
}

Handle PassBuilder::create(TextureInfo info, Access access) {
    Handle handle = mFrameGraph.texture(info, access);
    mRenderPass.creates.push_back({handle, access});
    return handle;
}

void PassBuilder::side_effects(bool effects) {
    mRenderPass.has_side_effects = effects;
}

bool FrameGraph::is_imported(Handle handle) const {
    return mHandles[handle.index].type == ResourceType::eImported;
}

uint FrameGraph::add_handle(ResourceHandle handle, Access access) {
    uint index = mResources.size();
    mHandles.emplace_back(handle);
    return index;
}

Handle FrameGraph::texture(TextureInfo info, Access access) {
    ResourceHandle handle = {
        .info = info,
        .type = ResourceType::eTransient,
        .access = access,
        .resource = nullptr,
    };

    uint index = add_handle(handle, access);
    return {index};
}

Handle FrameGraph::include(sm::StringView name, TextureInfo info, Access access, ID3D12Resource *resource) {
    ResourceHandle handle = {
        .info = info,
        .type = ResourceType::eImported,
        .access = access,
        .resource = resource
    };

    uint index = add_handle(handle, access);
    return {index};
}

PassBuilder FrameGraph::pass(sm::StringView name) {
    RenderPass pass;
    pass.name = name;

    auto& ref = mRenderPasses.emplace_back(pass);
    return { *this, ref };
}

void FrameGraph::compile() {
    // cull all the nodes that are not used
    for (auto& pass : mRenderPasses) {
        pass.refcount = (uint)pass.writes.size();

        for (auto& [index, access] : pass.reads) {
            mHandles[index.index].refcount += 1;
        }

        for (auto& [index, access] : pass.writes) {
            mHandles[index.index].producer = &pass;
        }
    }

    // culling
    sm::Stack<ResourceHandle*> queue;
    for (auto& handle : mHandles) {
        if (handle.refcount == 0) {
            queue.push(&handle);
        }
    }

    while (!queue.empty()) {
        ResourceHandle *handle = queue.top();
        queue.pop();
        RenderPass *producer = handle->producer;
        if (producer == nullptr || producer->has_side_effects) {
            continue;
        }

        producer->refcount -= 1;
        if (producer->refcount == 0) {
            for (auto& [index, access] : producer->reads) {
                auto& handle = mHandles[index.index];
                handle.refcount -= 1;
                if (handle.refcount == 0) {
                    queue.push(&handle);
                }
            }
        }
    }

    for (auto& pass : mRenderPasses) {
        if (pass.refcount == 0) continue;

        for (auto& [index, access] : pass.creates)
            mHandles[index.index].producer = &pass;
        for (auto& [index, access] : pass.reads)
            mHandles[index.index].last = &pass;
        for (auto& [index, access] : pass.writes)
            mHandles[index.index].last = &pass;
    }
}

void FrameGraph::execute() {
    sm::HashMap<ID3D12Resource*, D3D12_RESOURCE_STATES> states;

    sm::SmallVector<D3D12_RESOURCE_BARRIER, 4> barriers;

    auto& commands = mContext.mCommandList;
    for (auto& pass : mRenderPasses) {
        if (!pass.should_execute()) continue;

        for (auto& [index, access] : pass.creates) {
            ID3D12Resource *resource = mHandles[index.index].resource;
            states[resource] = access.as_facade();
        }

        for (auto& [index, access] : pass.reads) {
            ID3D12Resource *resource = mHandles[index.index].resource;
            states[resource] = access.as_facade();
        }

        for (auto& [index, access] : pass.writes) {
            ID3D12Resource *resource = mHandles[index.index].resource;
            states[resource] = access.as_facade();
        }

        if (barriers.size() > 0) {
            commands->ResourceBarrier(barriers.size(), barriers.data());
            barriers.clear();
        }

        pass.execute(*this, mContext);
    }
}
