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

PassBuilder FrameGraph::pass(sm::StringView name, render::CommandListType type) {
    RenderPass pass = { .type = type, .name = sm::String{name} };

    auto& ref = mRenderPasses.emplace_back(std::move(pass));
    return { *this, ref };
}

PassBuilder FrameGraph::graphics(sm::StringView name) {
    return pass(name, render::CommandListType::eDirect);
}

PassBuilder FrameGraph::compute(sm::StringView name) {
    return pass(name, render::CommandListType::eCompute);
}

PassBuilder FrameGraph::copy(sm::StringView name) {
    return pass(name, render::CommandListType::eCopy);
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

static D3D12_RESOURCE_DESC build_desc(const ResourceInfo& info, D3D12_RESOURCE_FLAGS flags) {
    if (info.sz.type == ResourceSize::eBuffer)
        return CD3DX12_RESOURCE_DESC::Buffer(info.sz.buffer_size, flags);

    return CD3DX12_RESOURCE_DESC::Tex2D(
        /*format=*/ info.format,
        /*width=*/ info.sz.tex2d_size.width,
        /*height=*/ info.sz.tex2d_size.height,
        /*arraySize=*/ 1,
        /*mipLevels=*/ 1,
        /*sampleCount=*/ 1,
        /*sampleQuality=*/ 0,
        /*flags=*/ flags
    );
}

void FrameGraph::create_resources() {
    mResources.clear();

    for (auto& handle : mHandles) {
        // skip resources that are not used
        if (!handle.is_used()) continue;
        if (handle.is_imported()) continue;

        const auto& info = handle.info;

        const auto flags = get_required_flags(handle.access);

        D3D12_CLEAR_VALUE storage;
        auto *clear = info.clear.get_value(storage, info.format);

        auto desc = build_desc(info, flags);

        const auto state = handle.access.as_facade();

        auto& resource = mResources.emplace_back();
        SM_ASSERT_HR(mContext.create_resource(resource, D3D12_HEAP_TYPE_DEFAULT, desc, state, clear));

        resource.mResource.rename(handle.name);

        handle.resource = resource.get();
    }
}

void FrameGraph::create_resource_descriptors() {
    UsageTracker tracker;

    for (auto& pass : mRenderPasses) {
        if (!pass.is_used()) continue;

        for (auto& [_, index, access] : pass.reads) {
            tracker.access(resource(index), access.as_facade());
        }

        for (auto& [_, index, access] : pass.writes) {
            tracker.access(resource(index), access.as_facade());
        }

        for (auto& [_, index, access] : pass.creates) {
            tracker.access(resource(index), access.as_facade());
        }
    }

    auto& device = mContext.mDevice;

    for (auto& handle : mHandles) {
        if (!handle.is_used()) continue;
        if (handle.is_imported()) continue;

        if (tracker.needs_uav(handle.resource)) {
            handle.uav = mContext.mSrvPool.allocate();

            const auto srv_handle = mContext.mSrvPool.cpu_handle(handle.uav);
            device->CreateUnorderedAccessView(handle.resource, nullptr, nullptr, srv_handle);
        }

        if (tracker.needs_srv(handle.resource)) {
            handle.srv = mContext.mSrvPool.allocate();

            const auto srv_handle = mContext.mSrvPool.cpu_handle(handle.srv);
            device->CreateShaderResourceView(handle.resource, nullptr, srv_handle);
        }

        if (tracker.needs_dsv(handle.resource)) {
            handle.dsv = mContext.mDsvPool.allocate();

            const auto dsv_handle = mContext.mDsvPool.cpu_handle(handle.dsv);
            device->CreateDepthStencilView(handle.resource, nullptr, dsv_handle);
        }

        if (tracker.needs_rtv(handle.resource)) {
            handle.rtv = mContext.mRtvPool.allocate();

            const auto rtv_handle = mContext.mRtvPool.cpu_handle(handle.rtv);
            device->CreateRenderTargetView(handle.resource, nullptr, rtv_handle);
        }
    }
}

///
/// per frame command data
///

void FrameGraph::reset_frame_commands() {
    mFrameData.clear();
}

void FrameGraph::init_frame_commands() {
    auto& device = mContext.mDevice;
    uint length = mContext.mSwapChainConfig.length;
    for (auto& [type, allocators, commands] : mFrameData) {
        auto kind = type.as_facade();

        allocators.resize(length);

        for (uint i = 0; i < length; i++) {
            SM_ASSERT_HR(device->CreateCommandAllocator(kind, IID_PPV_ARGS(&allocators[i])));
        }

        SM_ASSERT_HR(device->CreateCommandList(0, kind, *allocators[0], nullptr, IID_PPV_ARGS(&commands)));
        SM_ASSERT_HR(commands->Close());
    }
}

CommandListHandle FrameGraph::add_commands(render::CommandListType type) {
    FrameCommandData data = { .type = type };
    uint index = mFrameData.size();
    mFrameData.emplace_back(std::move(data));
    return {index};
}

render::CommandListType FrameGraph::get_command_type(CommandListHandle handle) {
    return mFrameData[handle.index].type;
}

void FrameGraph::open_commands(CommandListHandle handle) {
    auto& [type, allocators, commands] = mFrameData[handle.index];

    uint index = mContext.mFrameIndex;
    ID3D12CommandAllocator *allocator = *allocators[index];

    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(commands->Reset(allocator, nullptr));

    // TODO: should we do this here?
    ID3D12DescriptorHeap *heaps[] = { mContext.mSrvPool.get() };
    commands->SetDescriptorHeaps(1, heaps);
}

void FrameGraph::close_commands(CommandListHandle handle) {
    auto& [_, _, commands] = mFrameData[handle.index];

    SM_ASSERT_HR(commands->Close());
}

ID3D12GraphicsCommandList1 *FrameGraph::get_commands(CommandListHandle handle) {
    return mFrameData[handle.index].commands.get();
}

///
/// execution
///

struct BarrierRecord {
    sm::Vector<events::ResourceBarrier::Transition> transitions;

    void transition(Handle handle, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
        if (from == to) return;

        transitions.push_back({handle, from, to});
    }

    void submit(FrameSchedule& schedule, CommandListHandle handle) {
        if (transitions.empty()) return;

        events::ResourceBarrier event{handle, transitions};
        schedule.push_back(event);

        transitions.clear();
    }
};

void FrameGraph::schedule_graph() {
    // track the expected state of each resource
    sm::HashMap<Handle, D3D12_RESOURCE_STATES> initial;
    BarrierRecord barriers;

    auto update_state = [&](Handle index, Access access) {
        D3D12_RESOURCE_STATES state = access.as_facade();

        if (initial.contains(index)) {
            barriers.transition(index, initial[index], state);
        }

        initial[index] = state;
    };

    // save the initial state of all resources
    // so they can be restored after the frame
    struct FinalState {
        Handle handle;
        D3D12_RESOURCE_STATES state;
    };
    sm::Vector<FinalState> restore;

    for (uint i = 0; i < mHandles.size(); i++) {
        auto& handle = mHandles[i];
        if (!handle.is_imported() || !handle.is_used()) continue;

        auto state = handle.access.as_facade();
        restore.push_back({Handle{i}, state});
        initial[Handle{i}] = state;
    }

    const render::CommandListType queue = [&] {
        for (auto& pass : mRenderPasses) {
            if (!pass.is_used()) continue;

            return pass.type;
        }

        CT_NEVER("All render passes are culled, no queue type found");
    }();

    CommandListHandle cmd = add_commands(queue);

    // open the initial command list
    events::OpenCommands open = {
        .handle = cmd
    };

    mFrameSchedule.push_back(open);

    for (uint i = 0; i < mRenderPasses.size(); i++) {
        auto& pass = mRenderPasses[i];
        if (!pass.is_used()) continue;

        // if the queue type changes, add a sync event
        if (pass.type != get_command_type(cmd)) {
            // submit the current queues work
            events::SubmitCommands submit = {
                .handle = cmd
            };

            mFrameSchedule.push_back(submit);

            // assume the next queue depends on the current one
            events::DeviceSync sync = {
                .signal = queue,
                .wait = pass.type
            };

            mFrameSchedule.push_back(sync);

            cmd = add_commands(pass.type);
        }

        for (auto& [_, index, access] : pass.creates) {
            restore.push_back({index, access.as_facade()});
            update_state(index, access);
        }

        for (auto& [_, index, access] : pass.reads) {
            update_state(index, access);
        }

        for (auto& [_, index, access] : pass.writes) {
            update_state(index, access);
        }

        // if we have any barriers, submit them
        barriers.submit(mFrameSchedule, cmd);

        // then record the commands for this pass
        events::RecordCommands record = {
            .handle = cmd,
            .pass = PassHandle{i}
        };

        mFrameSchedule.push_back(record);
    }

    // transition imported and created resources back to their initial state
    for (auto& [handle, state] : restore) {
        barriers.transition(handle, initial[handle], state);
    }

    barriers.submit(mFrameSchedule, cmd);

    // submit the last command list
    events::SubmitCommands submit = {
        .handle = cmd,
    };

    mFrameSchedule.push_back(submit);
}

void FrameGraph::destroy_resources() {
    for (auto& handle : mHandles) {
        if (handle.is_imported()) continue;

        mContext.mRtvPool.safe_release(handle.rtv);
        mContext.mDsvPool.safe_release(handle.dsv);
        mContext.mSrvPool.safe_release(handle.srv);
        mContext.mSrvPool.safe_release(handle.uav);
    }

    mResources.clear();
}

void FrameGraph::reset_device_data() {
    mDeviceData.clear();
}

void FrameGraph::reset() {
    mFrameSchedule.clear();
    reset_frame_commands();
    destroy_resources();
    mRenderPasses.clear();
    mHandles.clear();
}

void FrameGraph::compile() {
    optimize();
    create_resources();
    create_resource_descriptors();
    schedule_graph();
    init_frame_commands();
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

void FrameGraph::build_raw_events(events::ResourceBarrier& barrier) {
    for (size_t i = 0; i < barrier.size(); i++) {
        auto& detail = barrier.barriers[i];
        auto& raw = barrier.raw[i];

        raw = CD3DX12_RESOURCE_BARRIER::Transition(resource(detail.handle), detail.before, detail.after);
    }
}

template<typename... T>
struct overloaded : T... {
    using T::operator()...;
};

void FrameGraph::execute() {
    SM_UNUSED BYTE colour = PIX_COLOR_DEFAULT;

    for (auto& step : mFrameSchedule) {
        std::visit(overloaded {
            [&](events::DeviceSync sync) {
                ID3D12CommandQueue *signal = mContext.get_queue(sync.signal);
                ID3D12CommandQueue *wait = mContext.get_queue(sync.wait);

                ID3D12Fence *fence = mContext.mDeviceFence.get();
                uint64 value = mContext.mDeviceFenceValue++;

                SM_ASSERT_HR(signal->Signal(fence, value));
                SM_ASSERT_HR(wait->Wait(fence, value));
            },
            [&](events::ResourceBarrier& barrier) {
                build_raw_events(barrier);
                ID3D12GraphicsCommandList1 *commands = get_commands(barrier.handle);
                commands->ResourceBarrier(barrier.size(), barrier.data());
            },
            [&](events::OpenCommands open) {
                open_commands(open.handle);
            },
            [&](events::RecordCommands record) {
                ID3D12GraphicsCommandList1 *commands = get_commands(record.handle);
                auto& pass = mRenderPasses[record.pass.index];
                RenderContext ctx{mContext, *this, commands};

                PIXBeginEvent(commands, PIX_COLOR_INDEX(colour++), "%s", pass.name.c_str());
                pass.execute(ctx);
                PIXEndEvent(commands);
            },
            [&](events::SubmitCommands submit) {
                close_commands(submit.handle);
                ID3D12GraphicsCommandList1 *commands = get_commands(submit.handle);
                ID3D12CommandQueue *queue = mContext.get_queue(get_command_type(submit.handle));

                ID3D12CommandList *lists[] = { commands };
                queue->ExecuteCommandLists(1, lists);
            }
        }, step);
    }

#if 0
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

        RenderContext ctx{mContext, *this, cmd};

        pass.execute(ctx);

        PIXEndEvent(cmd);
    }

    // transition imported and created resources back to their initial state
    for (auto& [resource, state] : restoration) {
        barriers.transition(resource, states[resource], state);
    }

    barriers.submit(commands.get());
#endif
}
