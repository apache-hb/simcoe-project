#include "stdafx.hpp"

#include "render/graph.hpp"

#include "render/render.hpp"

using namespace sm;
using namespace sm::graph;

using enum render::ResourceState::Inner;

using PassBuilder = FrameGraph::PassBuilder;

LOG_CATEGORY_IMPL(gRenderLog, "render");

math::float4 Clear::getClearColour() const {
    CTASSERTF(mClearType == ClearType::eColour, "Clear value is not a colour (%d)", std::to_underlying(mClearType));
    return mClearColour;
}

float Clear::getClearDepth() const {
    CTASSERTF(mClearType == ClearType::eDepth, "Clear value is not a depth (%d)", std::to_underlying(mClearType));
    return mClearDepth;
}

ClearType Clear::getClearType() const {
    return mClearType;
}

DXGI_FORMAT Clear::getFormat() const {
    return mFormat;
}

Clear Clear::empty() {
    Clear clear;
    clear.mClearType = ClearType::eEmpty;
    return clear;
}

Clear Clear::colour(math::float4 value, DXGI_FORMAT format) {
    Clear clear;
    clear.mClearType = ClearType::eColour;
    clear.mFormat = format;
    clear.mClearColour = value;
    return clear;
}

Clear Clear::depthStencil(float depth, uint8 stencil, DXGI_FORMAT format) {
    Clear clear;
    clear.mClearType = ClearType::eDepth;
    clear.mFormat = format;
    clear.mClearDepth = depth;
    clear.mClearStencil = stencil;
    return clear;
}

static constexpr D3D12_CLEAR_VALUE getClearValue(const Clear& clear) {
    switch (clear.getClearType()) {
    case ClearType::eDepth:
        return CD3DX12_CLEAR_VALUE(clear.getFormat(), clear.getClearDepth(), 0);
    case ClearType::eColour:
        return CD3DX12_CLEAR_VALUE(clear.getFormat(), clear.getClearColour().data());
    case ClearType::eEmpty:
        return CD3DX12_CLEAR_VALUE();
    }
}

void PassBuilder::add_write(Handle handle, sm::StringView name, Usage access) {
    mRenderPass.writes.push_back({sm::String{name}, handle, access});
}

FrameGraph::AccessBuilder PassBuilder::read(Handle handle, sm::StringView name, Usage access) {
    mRenderPass.reads.push_back({sm::String{name}, handle, access});

    return FrameGraph::AccessBuilder{mFrameGraph, handle};
}

FrameGraph::AccessBuilder PassBuilder::write(Handle handle, sm::StringView name, Usage access) {
    if (mFrameGraph.is_imported(handle)) {
        side_effects(true);
    }

    add_write(handle, name, access);

    return FrameGraph::AccessBuilder{mFrameGraph, handle};
}

FrameGraph::AccessBuilder PassBuilder::create(ResourceInfo info, sm::StringView name, Usage access) {
    auto id = fmt::format("{}/{}", mRenderPass.name, name);
    Handle handle = mFrameGraph.add_transient(info, access, id);
    mRenderPass.creates.push_back({id, handle, access});
    add_write(handle, name, access);
    return FrameGraph::AccessBuilder{mFrameGraph, handle};
}

void PassBuilder::side_effects(bool effects) {
    mRenderPass.has_side_effects = effects;
}

bool FrameGraph::is_imported(Handle handle) const {
    return mHandles[handle.index].type == ResourceType::eImported;
}

uint FrameGraph::add_handle(ResourceHandle handle, Usage access) {
    uint index = mHandles.size();
    mHandles.emplace_back(std::move(handle));
    return index;
}

Handle FrameGraph::add_transient(ResourceInfo info, Usage access, sm::StringView name) {
    ResourceHandle handle = {
        .name = sm::String{name},
        .info = info,
        .type = ResourceType::eTransient,
        .access = access,
        .resource = nullptr,
    };

    return {add_handle(handle, access)};
}

Handle FrameGraph::include(sm::StringView name, ResourceInfo info, Usage access, ID3D12Resource *resource) {
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
    RenderPass pass = { .name = sm::String{name}, .type = type };

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

        for (auto& access : pass.reads) {
            mHandles[access.index.index].refcount += 1;
        }

        for (auto& access : pass.writes) {
            mHandles[access.index.index].producer = &pass;
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

        for (auto& access : producer->reads) {
            auto& handle = mHandles[access.index.index];
            handle.refcount -= 1;
            if (handle.refcount == 0) {
                queue.push(&handle);
            }
        }
    }

    for (auto& pass : mRenderPasses) {
        if (pass.refcount == 0)
            continue;

        for (auto& access : pass.creates) {
            auto& handle = mHandles[access.index.index];
            handle.producer = &pass;
        }

        for (auto& access : pass.reads) {
            auto& handle = mHandles[access.index.index];
            handle.last = &pass;
        }

        for (auto& access : pass.writes) {
            auto& handle = mHandles[access.index.index];
            handle.last = &pass;
            handle.refcount += 1;
        }
    }
}

static constexpr D3D12_RESOURCE_STATES getInitialUsageState(Usage usage) {
    using enum Usage::Inner;
    switch (usage) {
    case ePresent: return D3D12_RESOURCE_STATE_PRESENT;
    case eUnknown: return D3D12_RESOURCE_STATE_COMMON;

    case ePixelShaderResource: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    case eTextureRead: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case eTextureWrite: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    case eRenderTarget: return D3D12_RESOURCE_STATE_RENDER_TARGET;

    case eCopySource: return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case eCopyTarget: return D3D12_RESOURCE_STATE_COPY_DEST;

    case eBufferRead:
    case eBufferWrite: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    case eDepthRead: return D3D12_RESOURCE_STATE_DEPTH_READ;
    case eDepthWrite: return D3D12_RESOURCE_STATE_DEPTH_WRITE;

    case eIndexBuffer: return D3D12_RESOURCE_STATE_INDEX_BUFFER;
    case eVertexBuffer: return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    }
}

using AccessBuilder = FrameGraph::AccessBuilder;

AccessBuilder& AccessBuilder::override_desc(D3D12_SHADER_RESOURCE_VIEW_DESC desc) {
    mFrameGraph.mHandles[mHandle.index].srv_desc = desc;

    return *this;
}

AccessBuilder& AccessBuilder::override_desc(D3D12_UNORDERED_ACCESS_VIEW_DESC desc) {
    mFrameGraph.mHandles[mHandle.index].uav_desc = desc;

    return *this;
}

AccessBuilder& AccessBuilder::override_desc(D3D12_RENDER_TARGET_VIEW_DESC desc) {
    mFrameGraph.mHandles[mHandle.index].rtv_desc = desc;

    return *this;
}

AccessBuilder& AccessBuilder::override_desc(D3D12_DEPTH_STENCIL_VIEW_DESC desc) {
    mFrameGraph.mHandles[mHandle.index].dsv_desc = desc;

    return *this;
}

struct UsageTracker {
    struct UsageBits {
        bool uav : 1 = false;
        bool srv : 1 = false;
        bool rtv : 1 = false;
        bool dsv : 1 = false;
    };

    sm::HashMap<uint, UsageBits> states;

    UsageBits tryGetUsage(uint resource) const {
        if (states.contains(resource)) {
            return states.at(resource);
        }

        return {};
    }

    void recordResourceAccess(uint resource, Usage usage) {
        using enum Usage::Inner;
        switch (usage) {
        case ePixelShaderResource:
        case eTextureRead:
            states[resource].srv = true;
            break;

        case eTextureWrite:
        case eBufferRead:
        case eBufferWrite:
            states[resource].uav = true;
            break;

        case eRenderTarget:
            states[resource].rtv = true;
            break;

        case eDepthRead:
        case eDepthWrite:
            states[resource].dsv = true;
            break;

        default:
            break;
        }
    }

    bool needsUav(uint resource) const {
        return tryGetUsage(resource).uav;
    }

    bool needsSrv(uint resource) const {
        return tryGetUsage(resource).srv;
    }

    bool needsRtv(uint resource) const {
        return tryGetUsage(resource).rtv;
    }

    bool needsDsv(uint resource) const {
        return tryGetUsage(resource).dsv;
    }

    D3D12_RESOURCE_FLAGS getResourceFlags(uint resource) const {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        UsageBits usage = tryGetUsage(resource);
        if (usage.uav)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        if (usage.dsv)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        if (usage.rtv)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        if (!usage.srv && !usage.uav)
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        return flags;
    }
};

void FrameGraph::createManagedResources() {
    mResources.clear();
    UsageTracker tracker;

    auto createNewResource = [&](const ResourceInfo& info, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state) {
        render::Resource resource;

        auto clear = getClearValue(info.clear);
        D3D12_CLEAR_VALUE *ptr = clear.Format == DXGI_FORMAT_UNKNOWN ? nullptr : &clear;

        ResourceSize size = info.size;

        if (size.type == ResourceSize::eBuffer) {
            SM_ASSERT_HR(mContext.createBufferResource(resource, D3D12_HEAP_TYPE_DEFAULT, size.buffer_size, D3D12_RESOURCE_STATE_COMMON, flags));
        } else {
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                /*format=*/ info.format,
                /*width=*/ size.tex2d_size.width,
                /*height=*/ size.tex2d_size.height,
                /*arraySize=*/ 1,
                /*mipLevels=*/ 1,
                /*sampleCount=*/ 1,
                /*sampleQuality=*/ 0,
                /*flags=*/ flags
            );
            SM_ASSERT_HR(mContext.createTextureResource(resource, D3D12_HEAP_TYPE_DEFAULT, desc, state, ptr));
        }

        return resource;
    };

    for (uint i = 0; i < mHandles.size(); i++) {
        auto& handle = mHandles[i];
        if (!handle.is_used() || handle.is_imported()) continue;

        tracker.recordResourceAccess(i, handle.access);
    }

    for (auto& pass : mRenderPasses) {
        if (!pass.is_used()) continue;

        pass.foreach(eRead | eWrite | eCreate, [&](const ResourceAccess& access) {
            tracker.recordResourceAccess(access.index.index, access.usage);
        });
    }

    for (uint i = 0; i < mHandles.size(); i++) {
        // skip resources that we don't need to create
        auto& handle = mHandles[i];
        if (!handle.is_used() || handle.is_imported()) continue;

        D3D12_RESOURCE_FLAGS flags = tracker.getResourceFlags(i);
        D3D12_RESOURCE_STATES state = getInitialUsageState(handle.access);

        gRenderLog.info("Create resource {} with state {}", handle.name, handle.access);
        auto resource = createNewResource(handle.info, flags, state);
        resource.rename(handle.name);

        handle.resource = resource.get();

        mResources.emplace_back(std::move(resource));
    }

    auto device = mContext.getDevice();

    for (uint i = 0; i < mHandles.size(); i++) {
        auto& handle = mHandles[i];
        if (!handle.is_managed()) continue;

        if (tracker.needsSrv(i)) {
            handle.srv = mContext.mSrvPool.allocate();

            const auto srv_handle = mContext.mSrvPool.cpu_handle(handle.srv);
            D3D12_SHADER_RESOURCE_VIEW_DESC *desc = handle.srv_desc ? &*handle.srv_desc : nullptr;
            device->CreateShaderResourceView(handle.resource, desc, srv_handle);
        }

        if (tracker.needsUav(i)) {
            handle.uav = mContext.mSrvPool.allocate();

            const auto uav_handle = mContext.mSrvPool.cpu_handle(handle.uav);
            D3D12_UNORDERED_ACCESS_VIEW_DESC *desc = handle.uav_desc ? &*handle.uav_desc : nullptr;
            device->CreateUnorderedAccessView(handle.resource, nullptr, desc, uav_handle);
        }

        if (tracker.needsDsv(i)) {
            handle.dsv = mContext.mDsvPool.allocate();

            const auto dsv_handle = mContext.mDsvPool.cpu_handle(handle.dsv);
            D3D12_DEPTH_STENCIL_VIEW_DESC *desc = handle.dsv_desc ? &*handle.dsv_desc : nullptr;
            device->CreateDepthStencilView(handle.resource, desc, dsv_handle);
        }

        if (tracker.needsRtv(i)) {
            handle.rtv = mContext.mRtvPool.allocate();

            const auto rtv_handle = mContext.mRtvPool.cpu_handle(handle.rtv);
            D3D12_RENDER_TARGET_VIEW_DESC *desc = handle.rtv_desc ? &*handle.rtv_desc : nullptr;
            device->CreateRenderTargetView(handle.resource, desc, rtv_handle);
        }
    }
}

///
/// per frame command data
///

void FrameGraph::FrameCommandData::resize(ID3D12Device1 *device, uint length) {
    for (uint i = 0; i < allocators.length(); i++) {
        allocators[i].reset();
    }

    allocators.resize(length);

    for (uint i = 0; i < length; i++) {
        SM_ASSERT_HR(device->CreateCommandAllocator(type.as_facade(), IID_PPV_ARGS(&allocators[i])));
        allocators[i].rename(fmt::format("FrameCommandData<{}>::allocator[{}]", type, i));
    }

    commands.reset();

    SM_ASSERT_HR(device->CreateCommandList(0, type.as_facade(), *allocators[0], nullptr, IID_PPV_ARGS(&commands)));
    SM_ASSERT_HR(commands->Close());

    commands.rename(fmt::format("FrameCommandData<{}>::commands", type));
}

void FrameGraph::reset_frame_commands() {
    mFrameData.clear();
}

void FrameGraph::init_frame_commands() {
    auto device = mContext.getDevice();
    uint length = mContext.getSwapChainLength();
    for (auto& [type, allocators, commands] : mFrameData) {
        allocators.resize(length);

        for (uint i = 0; i < length; i++) {
            SM_ASSERT_HR(device->CreateCommandAllocator(type.as_facade(), IID_PPV_ARGS(&allocators[i])));
            allocators[i].rename(fmt::format("FrameCommandData<{}>::allocator[{}]", type, i));
        }

        SM_ASSERT_HR(device->CreateCommandList(0, type.as_facade(), *allocators[0], nullptr, IID_PPV_ARGS(&commands)));
        SM_ASSERT_HR(commands->Close());

        commands.rename(fmt::format("FrameCommandList<{}>::commands", type));
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

void FrameGraph::resetCommandBuffer(CommandListHandle handle) {
    auto& [type, allocators, commands] = mFrameData[handle.index];

    uint index = mContext.mFrameIndex;
    ID3D12CommandAllocator *allocator = *allocators[index];

    SM_ASSERT_HR(allocator->Reset());
    SM_ASSERT_HR(commands->Reset(allocator, nullptr));

    // TODO: should we do this here?
    if (type != render::CommandListType::eCopy) {
        ID3D12DescriptorHeap *heaps[] = { mContext.mSrvPool.get() };
        commands->SetDescriptorHeaps(1, heaps);
    }
}

void FrameGraph::closeCommandBuffer(CommandListHandle handle) {
    auto& [_, _, commands] = mFrameData[handle.index];

    SM_ASSERT_HR(commands->Close());
}

ID3D12GraphicsCommandList1 *FrameGraph::getCommandBuffer(CommandListHandle handle) {
    return mFrameData[handle.index].commands.get();
}

///
/// fences
///

FenceHandle FrameGraph::addFence(sm::StringView name) {
    auto device = mContext.getDevice();
    FenceData data;
    SM_ASSERT_HR(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&data.fence)));
    data.fence.rename(name);

    uint index = mFences.size();
    mFences.emplace_back(std::move(data));

    return {index};
}

FrameGraph::FenceData& FrameGraph::getFence(FenceHandle handle) {
    return mFences[handle.index];
}

void FrameGraph::clearFences() {
    mFences.clear();
}

///
/// execution
///

void FrameGraph::schedule_graph() {
    struct BarrierRecord {
        FrameGraph& graph;
        sm::Vector<events::ResourceBarrier::Transition> transitions;

        BarrierRecord(FrameGraph& graph)
            : graph(graph)
        { }

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

    struct ScheduleBuilder {
        FrameGraph& self;
        FrameSchedule& schedule;

        CommandListHandle openCommandBuffer(render::CommandListType type) {
            CommandListHandle handle = self.add_commands(type);

            events::OpenCommands open = {
                .handle = handle
            };

            schedule.push_back(open);

            return handle;
        }

        void submitCommandBuffer(CommandListHandle handle) {
            events::SubmitCommands submit = {
                .handle = handle
            };

            schedule.push_back(submit);
        }

        void recordCommands(CommandListHandle handle, PassHandle pass) {
            events::RecordCommands record = {
                .handle = handle,
                .pass = pass
            };

            schedule.push_back(record);
        }
    } builder = { *this, mFrameSchedule };

    // track the expected state of each resource
    sm::HashMap<Handle, D3D12_RESOURCE_STATES> initial;
    BarrierRecord barriers{*this};

    auto update_state = [&](Handle index, Usage access) {
        D3D12_RESOURCE_STATES state = getInitialUsageState(access);

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

        auto state = getInitialUsageState(handle.access);
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

    CommandListHandle cmd = builder.openCommandBuffer(queue);

    sm::Vector<RenderPass> stack;

    std::array<bool, D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE + 1> synced = { false };

    for (uint i = 0; i < mRenderPasses.size(); i++) {
        auto& pass = mRenderPasses[i];
        if (!pass.is_used()) continue;

        // if the queue type changes, add a sync event
        if (pass.type != get_command_type(cmd)) {
            // submit the current queues work
            builder.submitCommandBuffer(cmd);

            for (auto& p : stack) {
                if (pass.depends_on(p) && pass.type != p.type) {
                    D3D12_COMMAND_LIST_TYPE ty = p.type.as_facade();
                    if (!synced[ty]) {
                        events::DeviceSync sync = {
                            .signal = p.type,
                            .wait = pass.type,
                            .fence = addFence(fmt::format("sync {} -> {}", p.name, pass.name))
                        };

                        mFrameSchedule.push_back(sync);

                        synced[ty] = true;
                    }
                }
            }

            synced.fill(false);

            // open a new command buffer
            cmd = builder.openCommandBuffer(pass.type);

            stack.push_back(pass);
        }

        stack.push_back(pass);

        for (auto& access : pass.creates) {
            restore.push_back({access.index, getInitialUsageState(access.usage)});
            update_state(access.index, access.usage);
        }

        pass.foreach(eRead | eWrite, [&](const ResourceAccess& access) {
            update_state(access.index, access.usage);
        });

        // TODO: do we need to handle UAV barriers or is our stuff
        // so pessemistic with waits that we don't need to worry about it?

        // if we have any barriers, submit them
        barriers.submit(mFrameSchedule, cmd);

        builder.recordCommands(cmd, PassHandle{i});

        // only add early barriers on direct command list
        // not sure of all the special cases yet
        if (get_command_type(cmd) != render::CommandListType::eDirect)
            continue;

        // if we can add barriers for the next pass, do so
        pass.foreach(eRead | eWrite, [&](const auto& access) {
            auto n = find_next_use(i, access.index);
            if (!n.is_valid()) return;

            auto state = mRenderPasses[n.index].get_handle_usage(access.index);
            update_state(access.index, state);
        });

        // if we have any barriers, submit them
        barriers.submit(mFrameSchedule, cmd);
    }

    // transition imported and created resources back to their initial state
    for (auto& [handle, state] : restore) {
        barriers.transition(handle, initial[handle], state);
    }

    barriers.submit(mFrameSchedule, cmd);

    // submit the last command list
    builder.submitCommandBuffer(cmd);

    // TODO: need to sync all unsynced work here
}

void FrameGraph::destroyManagedResources() {
    for (auto& handle : mHandles) {
        if (handle.is_imported()) continue;

        mContext.mSrvPool.safe_release(handle.srv);
        mContext.mSrvPool.safe_release(handle.uav);
        mContext.mDsvPool.safe_release(handle.dsv);
        mContext.mRtvPool.safe_release(handle.rtv);
    }

    mResources.clear();
}

void FrameGraph::reset_device_data() {
    mDeviceData.clear();
}

void FrameGraph::resize_frame_data(uint size) {
    for (auto& it : mFrameData) {
        it.resize(mContext.getDevice(), size);
    }
}

void FrameGraph::reset() {
    mFrameSchedule.clear();
    reset_frame_commands();
    clearFences();
    destroyManagedResources();
    mRenderPasses.clear();
    mHandles.clear();
}

void FrameGraph::compile() {
    optimize();
    createManagedResources();
    schedule_graph();
    init_frame_commands();
}

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
                gRenderLog.info("Sync {} -> {}", sync.signal, sync.wait);
                ID3D12CommandQueue *signal = mContext.getQueue(sync.signal);
                ID3D12CommandQueue *wait = mContext.getQueue(sync.wait);
                auto& [fence, value] = getFence(sync.fence);

                uint64 it = value++;

                SM_ASSERT_HR(signal->Signal(fence.get(), it));
                SM_ASSERT_HR(wait->Wait(fence.get(), it));
            },
            [&](events::ResourceBarrier& barrier) {
                gRenderLog.info("ResourceBarrier {} barriers {}.{}", barrier.size(), mFrameData[barrier.handle.index].type, barrier.handle.index);
                build_raw_events(barrier);
                ID3D12GraphicsCommandList1 *commands = getCommandBuffer(barrier.handle);
                commands->ResourceBarrier(barrier.size(), barrier.data());
            },
            [&](events::OpenCommands open) {
                gRenderLog.info("OpenCommands {}.{}", mFrameData[open.handle.index].type, open.handle.index);
                resetCommandBuffer(open.handle);
            },
            [&](events::RecordCommands record) {
                gRenderLog.info("RecordCommands {}.{}", mRenderPasses[record.pass.index].name, record.pass.index);
                ID3D12GraphicsCommandList1 *commands = getCommandBuffer(record.handle);
                auto& pass = mRenderPasses[record.pass.index];
                RenderContext ctx{getContext(), *this, pass, commands};

                PIXBeginEvent(commands, PIX_COLOR_INDEX(colour++), "%s", pass.name.c_str());
                pass.execute(ctx);
                PIXEndEvent(commands);
            },
            [&](events::SubmitCommands submit) {
                gRenderLog.info("SubmitCommands {}.{}", mFrameData[submit.handle.index].type, submit.handle.index);
                closeCommandBuffer(submit.handle);
                ID3D12GraphicsCommandList1 *commands = getCommandBuffer(submit.handle);
                ID3D12CommandQueue *queue = mContext.getQueue(get_command_type(submit.handle));

                ID3D12CommandList *lists[] = { commands };
                queue->ExecuteCommandLists(1, lists);
            }
        }, step);
    }

    // make all fences signal mDirectQueue
    for (auto& [fence, value] : mFences) {
        ID3D12CommandQueue *queue = mContext.getQueue(render::CommandListType::eDirect);

        uint64 it = value++;

        SM_ASSERT_HR(queue->Signal(fence.get(), it));
        SM_ASSERT_HR(queue->Wait(fence.get(), it));
    }
}

#if 0
struct GraphSorter {
        sm::Vector<RenderPass>& passes;

        sm::Vector<uint> indices;
        sm::Vector<sm::Set<uint>> adjacency;

        GraphSorter(sm::Vector<RenderPass>& passes)
            : passes(passes)
        {
            for (uint i = 0; i < passes.size(); i++) {
                if (passes[i].is_used())
                    indices.push_back(i);
            }

            build_adjacency();
        }

        void build_adjacency() {
            adjacency.resize(indices.size());

            for (uint i = 0; i < indices.size(); i++) {
                auto& pass = passes[indices[i]];

                for (auto& [_, index, _] : pass.reads) {
                    adjacency[i].emplace(index.index);
                }

                for (auto& [_, index, _] : pass.writes) {
                    adjacency[i].emplace(index.index);
                }

                for (auto& [_, index, _] : pass.creates) {
                    adjacency[i].emplace(index.index);
                }
            }
        }

        void dfs(uint current,
                 sm::Vector<bool>& visited,
                 sm::Vector<int>& departure,
                 int& time) {

            visited[current] = true;

            time += 1;

            for (auto& index : adjacency[current]) {
                if (!visited[index]) {
                    dfs(index, visited, departure, time);
                }
            }

            departure[time] = current;
            time += 1;
        }

        sm::Vector<uint> toposort() {
#if 0
            sm::Vector<int> departure(indices.size() * 2, -1);
            sm::Vector<bool> visited(indices.size());
            int time = 0;

            for (uint i = 0; i < indices.size(); i++) {
                if (!visited[i]) {
                    dfs(i, visited, departure, time);
                }
            }

            sm::Vector<RenderPass> sorted;
            for (int i = indices.size() * 2 - 1; i >= 0; i--) {
                int index = departure[i];
                if (index == -1) continue;

                sorted.push_back(passes[indices[index]]);
            }
#endif

            // TODO: sorting is disabled for now
            sm::Vector<uint> sorted;
            for (uint i = 0; i < indices.size(); i++) {
                sorted.push_back(indices[i]);
            }

            return sorted;
        }
    };

    // collect all the used render passes
    GraphSorter sorter{mRenderPasses};

#endif
