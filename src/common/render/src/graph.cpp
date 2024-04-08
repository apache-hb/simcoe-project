#include "stdafx.hpp"

#include "render/graph.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::graph;

using enum render::ResourceState::Inner;

using PassBuilder = FrameGraph::PassBuilder;

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
    storage = { .Format = format };
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

void PassBuilder::add_write(Handle handle, sm::StringView name, Access access) {
    mRenderPass.writes.push_back({sm::String{name}, handle, access});
}

void PassBuilder::read(Handle handle, sm::StringView name, Access access) {
    mRenderPass.reads.push_back({sm::String{name}, handle, access});
}

void PassBuilder::write(Handle handle, sm::StringView name, Access access) {
    if (mFrameGraph.is_imported(handle)) {
        side_effects(true);
    }

    add_write(handle, name, access);
}

Handle PassBuilder::create(ResourceInfo info, sm::StringView name, Access access) {
    auto id = fmt::format("{}/{}", mRenderPass.name, name);
    Handle handle = mFrameGraph.texture(info, access, id);
    mRenderPass.creates.push_back({id, handle, access});
    add_write(handle, name, access);
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

Handle FrameGraph::texture(ResourceInfo info, Access access, sm::StringView name) {
    ResourceHandle handle = {
        .name = sm::String{name},
        .info = info,
        .type = ResourceType::eTransient,
        .access = access,
        .resource = nullptr,
    };

    uint index = add_handle(handle, access);
    return {index};
}

Handle FrameGraph::include(sm::StringView name, ResourceInfo info, Access access, ID3D12Resource *resource) {
    ResourceHandle handle = {
        .name = sm::String{name},
        .info = info,
        .type = resource ? ResourceType::eManaged : ResourceType::eImported,
        .access = access,
        .resource = resource
    };

    uint index = add_handle(handle, access);
    return {index};
}

void FrameGraph::update(Handle handle, ID3D12Resource *resource) {
    mHandles[handle.index].resource = resource;
}

void FrameGraph::update_rtv(Handle handle, render::RtvIndex rtv) {
    mHandles[handle.index].rtv = rtv;
}

void FrameGraph::update_dsv(Handle handle, render::DsvIndex dsv) {
    mHandles[handle.index].dsv = dsv;
}

void FrameGraph::update_srv(Handle handle, render::SrvIndex srv) {
    mHandles[handle.index].srv = srv;
}

void FrameGraph::update_uav(Handle handle, render::SrvIndex uav) {
    mHandles[handle.index].uav = uav;
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

render::SrvIndex FrameGraph::uav(Handle handle) {
    return mHandles[handle.index].uav;
}

PassBuilder FrameGraph::pass(sm::StringView name, RenderPass::Type type) {
    RenderPass pass;
    pass.type = type;
    pass.name = name;

    auto& ref = mRenderPasses.emplace_back(std::move(pass));
    return { *this, ref };
}

PassBuilder FrameGraph::graphics(sm::StringView name) {
    RenderPass pass;
    pass.type = RenderPass::eDirect;
    pass.name = name;

    auto& ref = mRenderPasses.emplace_back(std::move(pass));
    return { *this, ref };
}

PassBuilder FrameGraph::compute(sm::StringView name) {
    RenderPass pass;
    pass.type = RenderPass::eCompute;
    pass.name = name;

    auto& ref = mRenderPasses.emplace_back(std::move(pass));
    return { *this, ref };
}

PassBuilder FrameGraph::copy(sm::StringView name) {
    RenderPass pass;
    pass.type = RenderPass::eCopy;
    pass.name = name;

    auto& ref = mRenderPasses.emplace_back(std::move(pass));
    return { *this, ref };
}

void FrameGraph::optimize() {
    // cull all the nodes that are not used
    for (auto& pass : mRenderPasses) {
        pass.refcount = (uint)pass.writes.size();

        for (auto& [_, index, _] : pass.reads) {
            mHandles[index.index].refcount += 1;
        }

        for (auto& [_, index, _] : pass.writes) {
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
        if (producer->refcount != 0)
            continue;

        for (auto& [_, index, access] : producer->reads) {
            auto& handle = mHandles[index.index];
            handle.refcount -= 1;
            if (handle.refcount == 0) {
                queue.push(&handle);
            }
        }
    }

    for (auto& pass : mRenderPasses) {
        if (pass.refcount == 0)
            continue;

        for (auto& [_, index, access] : pass.creates) {
            auto& handle = mHandles[index.index];
            handle.producer = &pass;
        }

        for (auto& [_, index, access] : pass.reads) {
            auto& handle = mHandles[index.index];
            handle.last = &pass;
        }

        for (auto& [_, index, access] : pass.writes) {
            auto& handle = mHandles[index.index];
            handle.last = &pass;
            handle.refcount += 1;
        }
    }
}

static constexpr D3D12_RESOURCE_FLAGS get_required_flags(Access access) {
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    if (access == eRenderTarget) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    } else if (access == eDepthTarget || access == eDepthRead) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    } else if (access == ePixelShaderResource) {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    return flags;
}

struct UsageTracker {
    sm::HashMap<ID3D12Resource*, D3D12_RESOURCE_STATES> states;

    D3D12_RESOURCE_STATES at_or(ID3D12Resource *resource, D3D12_RESOURCE_STATES state) const {
        if (states.contains(resource)) {
            return states.at(resource);
        }

        return state;
    }

    void access(ID3D12Resource *resource, D3D12_RESOURCE_STATES state) {
        states[resource] |= state;
    }

    bool needs_uav(ID3D12Resource *resource) const {
        return (at_or(resource, D3D12_RESOURCE_STATE_COMMON) & D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    bool needs_srv(ID3D12Resource *resource) const {
        return (at_or(resource, D3D12_RESOURCE_STATE_COMMON) & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    bool needs_rtv(ID3D12Resource *resource) const {
        return (at_or(resource, D3D12_RESOURCE_STATE_COMMON) & D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    bool needs_dsv(ID3D12Resource *resource) const {
        return (at_or(resource, D3D12_RESOURCE_STATE_COMMON) & (D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ));
    }
};

void FrameGraph::create_resources() {
    mResources.clear();

    auto& device = mContext.mDevice;

    for (auto& handle : mHandles) {
        // skip resources that are not used
        if (!handle.is_used()) continue;
        if (handle.is_imported()) continue;

        const auto& info = handle.info;

        const auto flags = get_required_flags(handle.access);

        D3D12_CLEAR_VALUE storage;
        auto *clear = info.clear.get_value(storage, info.format);

        auto size = info.size;

        const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
            /*format=*/ info.format,
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

        resource.mResource.rename(handle.name);

        handle.resource = resource.get();

        // TODO: this seems a little weird
        // if (handle.access == eRenderTarget) {
        //     handle.rtv = mContext.mRtvPool.allocate();
        //     handle.srv = mContext.mSrvPool.allocate();

        //     const auto rtv_handle = mContext.mRtvPool.cpu_handle(handle.rtv);
        //     device->CreateRenderTargetView(handle.resource, nullptr, rtv_handle);

        //     const auto srv_handle = mContext.mSrvPool.cpu_handle(handle.srv);
        //     device->CreateShaderResourceView(handle.resource, nullptr, srv_handle);
        // } else if (handle.access == eDepthTarget) {
        //     handle.dsv = mContext.mDsvPool.allocate();

        //     const auto dsv_handle = mContext.mDsvPool.cpu_handle(handle.dsv);
        //     device->CreateDepthStencilView(handle.resource, nullptr, dsv_handle);
        // }
    }

    UsageTracker tracker;

    for (auto& pass : mRenderPasses) {
        if (!pass.is_used()) continue;

        for (auto& [_, index, access] : pass.reads) {
            logs::gRender.info("Read: {} {}", mHandles[index.index].name, access);
            tracker.access(resource(index), access.as_facade());
        }

        for (auto& [_, index, access] : pass.writes) {
            logs::gRender.info("Write: {} {}", mHandles[index.index].name, access);
            tracker.access(resource(index), access.as_facade());
        }

        for (auto& [_, index, access] : pass.creates) {
            logs::gRender.info("Create: {} {}", mHandles[index.index].name, access);
            tracker.access(resource(index), access.as_facade());
        }
    }

    for (auto& handle : mHandles) {
        if (!handle.is_used()) continue;
        if (handle.is_imported()) continue;

        if (tracker.needs_uav(handle.resource)) {
            logs::gRender.info("UAV: {}", handle.name);
            handle.uav = mContext.mSrvPool.allocate();

            const auto srv_handle = mContext.mSrvPool.cpu_handle(handle.uav);
            device->CreateUnorderedAccessView(handle.resource, nullptr, nullptr, srv_handle);
        }

        if (tracker.needs_srv(handle.resource)) {
            logs::gRender.info("SRV: {}", handle.name);
            handle.srv = mContext.mSrvPool.allocate();

            const auto srv_handle = mContext.mSrvPool.cpu_handle(handle.srv);
            device->CreateShaderResourceView(handle.resource, nullptr, srv_handle);
        }

        if (tracker.needs_dsv(handle.resource)) {
            logs::gRender.info("DSV: {}", handle.name);
            handle.dsv = mContext.mDsvPool.allocate();

            const auto dsv_handle = mContext.mDsvPool.cpu_handle(handle.dsv);
            device->CreateDepthStencilView(handle.resource, nullptr, dsv_handle);
        }

        if (tracker.needs_rtv(handle.resource)) {
            logs::gRender.info("RTV: {}", handle.name);

            handle.rtv = mContext.mRtvPool.allocate();

            const auto rtv_handle = mContext.mRtvPool.cpu_handle(handle.rtv);
            device->CreateRenderTargetView(handle.resource, nullptr, rtv_handle);
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

void FrameGraph::reset_device_data() {
    mDeviceData.clear();
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

struct BarrierList {
    sm::SmallVector<D3D12_RESOURCE_BARRIER, 4> barriers;

    void transition(ID3D12Resource *resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
        if (from == to) return;

        barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(resource, from, to));
    }

    uint length() const {
        return int_cast<uint>(barriers.size());
    }

    const D3D12_RESOURCE_BARRIER *data() const {
        return barriers.data();
    }

    void submit(ID3D12GraphicsCommandList *commands) {
        if (barriers.empty()) return;

        commands->ResourceBarrier(length(), data());

        barriers.clear();
    }
};

void FrameGraph::execute() {
    sm::HashMap<ID3D12Resource*, D3D12_RESOURCE_STATES> states;
    BarrierList barriers;
    auto& commands = mContext.mCommandList;

    auto update_state = [&](Handle index, Access access) {
        ID3D12Resource *resource = mHandles[index.index].resource;
        D3D12_RESOURCE_STATES state = access.as_facade();

        if (states.contains(resource)) {
            barriers.transition(resource, states[resource], state);
        }

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

    SM_UNUSED BYTE colour_index = PIX_COLOR_DEFAULT;
    for (auto& pass : mRenderPasses) {
        if (!pass.is_used()) continue;

        for (auto& [_, index, access] : pass.creates) {
            restoration.push_back({resource(index), access.as_facade()});

            update_state(index, access);
        }

        for (auto& [_, index, access] : pass.reads) {
            update_state(index, access);
        }

        for (auto& [_, index, access] : pass.writes) {
            update_state(index, access);
        }

        auto& cmd = commands.get();

        PIXBeginEvent(cmd, PIX_COLOR_INDEX(colour_index++), "%s", pass.name.c_str());

        barriers.submit(cmd);

        pass.execute(*this);

        PIXEndEvent(cmd);
    }

    // transition imported and created resources back to their initial state
    for (auto& [resource, state] : restoration) {
        barriers.transition(resource, states[resource], state);
    }

    barriers.submit(commands.get());
}
