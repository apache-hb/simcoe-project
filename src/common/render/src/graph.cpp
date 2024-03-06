#include "render/graph.hpp"

#include "render/render.hpp"

#include "core/map.hpp"
#include "core/stack.hpp"

#include "directx/d3dx12_core.h"

using namespace sm;
using namespace sm::graph;

using enum render::ResourceState::Inner;

using PassBuilder = FrameGraph::PassBuilder;

static auto gSink = logs::get_sink(logs::Category::eRender);

Clear graph::clear_colour(float4 colour) {
    Clear clear;
    clear.type = Clear::eColour;
    clear.colour = colour;
    return clear;
}

Clear graph::clear_depth(float depth) {
    Clear clear;
    clear.type = Clear::eDepth;
    clear.depth = depth;
    return clear;
}

D3D12_CLEAR_VALUE *Clear::get_value(D3D12_CLEAR_VALUE& storage, render::Format format) const {
    storage = { .Format = format.as_facade() };
    switch (type) {
    case eColour:
        storage.Color[0] = colour.r;
        storage.Color[1] = colour.g;
        storage.Color[2] = colour.b;
        storage.Color[3] = colour.a;
        break;
    case eDepth:
        storage.DepthStencil.Depth = depth;
        storage.DepthStencil.Stencil = 0;
        break;
    case eEmpty:
        return nullptr;
    }

    return &storage;
}

void PassBuilder::read(Handle handle, Access access) {
    mRenderPass.reads.push_back({handle, access});
}

void PassBuilder::write(Handle handle, Access access) {
    mRenderPass.writes.push_back({handle, access});
    if (mFrameGraph.is_imported(handle)) {
        side_effects(true);
        mRenderPass.has_side_effects = true;
    }
}

Handle PassBuilder::create(TextureInfo info, Access access) {
    Handle handle = mFrameGraph.texture(std::move(info), access);
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
    uint index = mHandles.size();
    mHandles.emplace_back(std::move(handle));
    return index;
}

Handle FrameGraph::texture(TextureInfo info, Access access) {
    ResourceHandle handle = {
        .info = std::move(info),
        .type = ResourceType::eTransient,
        .access = access,
        .resource = nullptr,
    };

    uint index = add_handle(handle, access);
    return {index};
}

Handle FrameGraph::include(TextureInfo info, Access access, ID3D12Resource *resource) {
    ResourceHandle handle = {
        .info = std::move(info),
        .type = resource ? ResourceType::eManaged : ResourceType::eImported,
        .access = access,
        .resource = resource
    };

    uint index = add_handle(handle, access);
    return {index};
}

uint2 FrameGraph::present_size() const {
    return mContext.mSwapChainSize;
}

uint2 FrameGraph::render_size() const {
    return mContext.mSceneSize;
}

void FrameGraph::update(Handle handle, ID3D12Resource *resource) {
    mHandles[handle.index].resource = resource;
}

void FrameGraph::update(Handle handle, render::RtvIndex rtv) {
    mHandles[handle.index].rtv = rtv;
}

void FrameGraph::update(Handle handle, render::DsvIndex dsv) {
    mHandles[handle.index].dsv = dsv;
}

void FrameGraph::update(Handle handle, render::SrvIndex srv) {
    mHandles[handle.index].srv = srv;
}

ID3D12Resource *FrameGraph::resource(Handle handle) {
    return mHandles[handle.index].resource;
}

render::RtvIndex FrameGraph::rtv(Handle handle) {
    return mHandles[handle.index].rtv;
}

render::DsvIndex FrameGraph::dsv(Handle handle) {
    return mHandles[handle.index].dsv;
}

render::SrvIndex FrameGraph::srv(Handle handle) {
    return mHandles[handle.index].srv;
}

PassBuilder FrameGraph::pass(sm::StringView name) {
    RenderPass pass;
    pass.name = name;

    auto& ref = mRenderPasses.emplace_back(pass);
    return { *this, ref };
}

void FrameGraph::optimize() {
    // cull all the nodes that are not used
    for (auto& pass : mRenderPasses) {
        pass.refcount = (uint)pass.writes.size();

        for (auto& [index, _] : pass.reads) {
            mHandles[index.index].refcount += 1;
        }

        for (auto& [index, _] : pass.writes) {
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

        for (auto& [index, access] : pass.creates) {
            auto& handle = mHandles[index.index];
            handle.producer = &pass;
            handle.refcount += 1;
        }

        for (auto& [index, access] : pass.reads) {
            auto& handle = mHandles[index.index];
            handle.last = &pass;
            handle.refcount += 1;
        }

        for (auto& [index, access] : pass.writes) {
            auto& handle = mHandles[index.index];
            handle.last = &pass;
            handle.refcount += 1;
        }
    }
}

static constexpr D3D12_RESOURCE_FLAGS get_required_flags(Access access) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (access.test(eRenderTarget)) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    } else if (access.test(eDepthTarget)) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    return flags;
}

void FrameGraph::create_resources() {
    mResources.clear();

    auto& device = mContext.mDevice;

    for (auto& handle : mHandles) {
        // skip resources that are not used
        if (!handle.is_used()) continue;

        const auto& info = handle.info;

        if (handle.is_imported()) continue;

        const auto flags = get_required_flags(handle.access);

        D3D12_CLEAR_VALUE storage;
        auto *clear = info.clear.get_value(storage, info.format);

        auto size = info.size;

        const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
            /*format=*/ info.format.as_facade(),
            /*width=*/ size.width,
            /*height=*/ size.height,
            /*arraySize=*/ 1,
            /*mipLevels=*/ 1,
            /*sampleCount=*/ 1,
            /*sampleQuality=*/ 0,
            /*flags=*/ flags
        );

        const auto state = handle.access.as_facade();

        auto& resource = mResources.emplace_back();
        SM_ASSERT_HR(mContext.create_resource(resource, D3D12_HEAP_TYPE_DEFAULT, desc, state, clear));

        resource.mResource.rename(info.name);

        handle.resource = resource.get();

        // TODO: this seems a little weird
        if (handle.access.test(eRenderTarget)) {
            handle.rtv = mContext.mRtvPool.allocate();
            handle.srv = mContext.mSrvPool.allocate();

            const auto rtv_handle = mContext.mRtvPool.cpu_handle(handle.rtv);
            device->CreateRenderTargetView(handle.resource, nullptr, rtv_handle);

            const auto srv_handle = mContext.mSrvPool.cpu_handle(handle.srv);
            device->CreateShaderResourceView(handle.resource, nullptr, srv_handle);
        } else if (handle.access.test(eDepthTarget)) {
            handle.dsv = mContext.mDsvPool.allocate();

            const auto dsv_handle = mContext.mDsvPool.cpu_handle(handle.dsv);
            device->CreateDepthStencilView(handle.resource, nullptr, dsv_handle);
        }
    }
}

void FrameGraph::destroy_resources() {
    for (auto& handle : mHandles) {
        if (handle.is_imported()) continue;

        mContext.mRtvPool.safe_release(handle.rtv);
        mContext.mDsvPool.safe_release(handle.dsv);
        mContext.mSrvPool.safe_release(handle.srv);
    }

    mResources.clear();
}

void FrameGraph::reset() {
    destroy_resources();
    mRenderPasses.clear();
    mHandles.clear();
}

void FrameGraph::compile() {
    optimize();
    create_resources();
}

void FrameGraph::execute() {
    sm::HashMap<ID3D12Resource*, D3D12_RESOURCE_STATES> states;
    auto& commands = mContext.mCommandList;

    auto update_state = [&](Handle index, Access access) {
        ID3D12Resource *resource = mHandles[index.index].resource;
        D3D12_RESOURCE_STATES state = access.as_facade();

        if (states.contains(resource))
            commands.transition(resource, states[resource], state);

        states[resource] = state;
    };

    struct FinalState {
        ID3D12Resource *resource;
        D3D12_RESOURCE_STATES state;
    };
    sm::SmallVector<FinalState, 8> restoration;

    // store the initial state of all imported resources
    for (auto& handle : mHandles) {
        if (!handle.is_imported() || !handle.is_used()) continue;

        auto state = handle.access.as_facade();

        restoration.push_back({handle.resource, state});
        states[handle.resource] = state;
    }

    for (auto& pass : mRenderPasses) {
        if (!pass.is_used()) continue;

        for (auto& [index, access] : pass.creates) {
            restoration.push_back({resource(index), access.as_facade()});

            update_state(index, access);
        }

        for (auto& [index, access] : pass.reads) {
            update_state(index, access);
        }

        for (auto& [index, access] : pass.writes) {
            update_state(index, access);
        }

        commands.submit_barriers();

        pass.execute(*this, mContext);
    }

    // transition imported and created resource back to their initial state
    for (auto& [resource, state] : restoration) {
        commands.transition(resource, states[resource], state);
    }

    commands.submit_barriers();
}
