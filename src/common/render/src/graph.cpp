#include "stdafx.hpp"

#include "render/graph.hpp"

#include "render/render.hpp"
#include <set>

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

ResourceAccess& PassBuilder::addWriteAccess(Handle handle, sm::StringView name, Usage access) {
    return mRenderPass.writes.emplace_back(ResourceAccess{sm::String{name}, handle, access});
}

FrameGraph::AccessBuilder PassBuilder::read(Handle handle, sm::StringView name, Usage access) {
    ResourceAccess& self = mRenderPass.reads.emplace_back(ResourceAccess{sm::String{name}, handle, access});

    return FrameGraph::AccessBuilder{mFrameGraph, self, handle};
}

FrameGraph::AccessBuilder PassBuilder::write(Handle handle, sm::StringView name, Usage access) {
    if (mFrameGraph.is_imported(handle)) {
        hasSideEffects(true);
    }

    ResourceAccess& self = addWriteAccess(handle, name, access);

    return FrameGraph::AccessBuilder{mFrameGraph, self, handle};
}

FrameGraph::AccessBuilder PassBuilder::create(ResourceInfo info, sm::StringView name, Usage access) {
    auto id = fmt::format("{}/{}", mRenderPass.name, name);
    Handle handle = mFrameGraph.add_transient(info, access, id);
    ResourceAccess& self = mRenderPass.creates.emplace_back(ResourceAccess{id, handle, access});
    addWriteAccess(handle, name, access);
    return FrameGraph::AccessBuilder{mFrameGraph, self, handle};
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

AccessBuilder& AccessBuilder::withLayout(D3D12_BARRIER_LAYOUT value) {
    mAccess.layout = value;
    return *this;
}

AccessBuilder& AccessBuilder::withAccess(D3D12_BARRIER_ACCESS value) {
    mAccess.access = value;
    return *this;
}

AccessBuilder& AccessBuilder::withSyncPoint(D3D12_BARRIER_SYNC value) {
    mAccess.sync = value;
    return *this;
}

static bool isUavRead(Usage usage) {
    using enum Usage::Inner;
    return usage == eTextureRead || usage == eBufferRead;
}

static bool isUavWrite(Usage usage) {
    using enum Usage::Inner;
    return usage == eTextureWrite || usage == eBufferWrite;
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

render::CommandListType FrameGraph::getCommandListType(CommandListHandle handle) {
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

ID3D12GraphicsCommandList1 *FrameGraph::getCommandList(CommandListHandle handle) {
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
    // responsible for adding new events to the frame schedule
    struct ScheduleBuilder {
        FrameGraph& self;
        FrameSchedule& schedule;

        CommandListHandle openCommandBuffer(render::CommandListType type) {
            CommandListHandle handle = self.add_commands(type);

            events::OpenCommands open = { .handle = handle };

            schedule.push_back(open);

            return handle;
        }

        void submitCommandBuffer(CommandListHandle handle) {
            events::SubmitCommands submit = { .handle = handle };

            schedule.push_back(submit);
        }

        void recordCommands(CommandListHandle handle, PassHandle pass) {
            events::RecordCommands record = {
                .handle = handle,
                .pass = pass
            };

            schedule.push_back(record);
        }

        void syncQueue(render::CommandListType signal, render::CommandListType wait) {
            events::DeviceSync sync = {
                .signal = signal,
                .wait = wait,
                .fence = self.addFence(fmt::format("sync {} -> {}", signal, wait))
            };

            schedule.push_back(sync);
        }
    } schedule = { *this, mFrameSchedule };

    // track the expected state of each resource
    sm::HashMap<Handle, D3D12_RESOURCE_STATES> initial;
    struct BarrierRecord {
        FrameGraph& graph;
        sm::Vector<D3D12_RESOURCE_BARRIER> barriers;

        BarrierRecord(FrameGraph& graph)
            : graph(graph)
        { }

        void transition(Handle handle, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
            if (before == after) return;

            barriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                graph.resource(handle),
                before,
                after
            ));
        }

        void uav(Handle handle) {
            barriers.push_back(CD3DX12_RESOURCE_BARRIER::UAV(graph.resource(handle)));
        }

        void submit(FrameSchedule& schedule, CommandListHandle handle) {
            if (barriers.empty()) return;

            events::ResourceBarrier event{handle, barriers};
            barriers.clear();

            schedule.push_back(event);
        }
    } barriers = { *this };

    auto updateResourceState = [&](Handle index, Usage access) {
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

        Handle idx{i};

        auto state = getInitialUsageState(handle.access);
        restore.push_back({idx, state});
        initial[idx] = state;
    }

    const render::CommandListType initialQueueType = [&] {
        for (auto& pass : mRenderPasses) {
            if (!pass.is_used()) continue;

            return pass.type;
        }

        CT_NEVER("All render passes are culled, no queue type found");
    }();

    CommandListHandle list = schedule.openCommandBuffer(initialQueueType);

    struct QueueSyncStatus {
        std::set<D3D12_COMMAND_LIST_TYPE> status;

        bool isQueueSynced(D3D12_COMMAND_LIST_TYPE type) {
            return status.contains(type);
        }

        void markQueueSynced(D3D12_COMMAND_LIST_TYPE type) {
            status.insert(type);
        }

        void reset() { status.clear(); }
    } queueSyncStatus;

    // all unsynchronized uav writes
    // assume that all uav reads want to be synchronized
    std::set<Handle> uavWrites;

    // all render passes that havent been synchronized
    sm::Vector<RenderPass> unsyncedPasses;

    for (uint i = 0; i < mRenderPasses.size(); i++) {
        auto& pass = mRenderPasses[i];
        if (!pass.is_used()) continue;

        // todo: go through the resource barriers and try and move them to direct command lists
        // its such a pain that no other queue can actually do most of the transitions.
    }

    for (uint i = 0; i < mRenderPasses.size(); i++) {
        auto& pass = mRenderPasses[i];
        if (!pass.is_used()) continue;

        // if the queue type changes, add a sync event
        if (pass.type != getCommandListType(list)) {
            // submit the current queues work
            schedule.submitCommandBuffer(list);

            // syncronize with all the queues we depend on
            for (auto& dep : unsyncedPasses) {
                if (pass.depends_on(dep) && pass.type != dep.type) {
                    D3D12_COMMAND_LIST_TYPE ty = dep.type.as_facade();

                    if (!queueSyncStatus.isQueueSynced(ty)) {
                        schedule.syncQueue(dep.type, pass.type);
                        queueSyncStatus.markQueueSynced(ty);
                    }
                }
            }

            queueSyncStatus.reset();

            // open a new command buffer
            list = schedule.openCommandBuffer(pass.type);
        }

        unsyncedPasses.push_back(pass);

        // sync any uav reads that are not synchronized for this pass
        pass.foreach(eRead, [&](const ResourceAccess& access) {
            if (isUavRead(access.usage) && uavWrites.contains(access.index)) {
                barriers.uav(access.index);
                uavWrites.erase(access.index);
            }
        });

        // mark all new uav writes as needing synchronization
        pass.foreach(eWrite, [&](const ResourceAccess& access) {
            if (isUavWrite(access.usage)) {
                uavWrites.insert(access.index);
            }
        });

        // mark the initial state of all resources this pass creates
        for (auto& access : pass.creates) {
            D3D12_RESOURCE_STATES state = getInitialUsageState(access.usage);
            restore.push_back({access.index, state});
            initial[access.index] = state;
        }

        // dont do any resource transitions on the copy queue
        // cant be bothered to try and figure out the edge cases
        auto type = getCommandListType(list);
        if (type == render::CommandListType::eCopy) {
            schedule.recordCommands(list, PassHandle{i});

            // only update the state of the resources used by this pass
            // TODO: this assumes that a copy pass will never read/write to a resource
            // it doesnt create.
            pass.foreach(eRead | eWrite, [&](const ResourceAccess& access) {
                initial[access.index] = getInitialUsageState(access.usage);
            });

            continue;
        } else if (type == render::CommandListType::eDirect) {
            pass.foreach(eRead | eWrite, [&](const ResourceAccess& access) {
                if (auto next = findNextHandleUse(i, access.index); next.is_valid()) {
                    if (mRenderPasses[next.index].type != render::CommandListType::eDirect) {
                        updateResourceState(access.index, access.usage);
                    }
                } else {
                    updateResourceState(access.index, access.usage);
                }
            });

        } else {
            pass.foreach(eRead | eWrite, [&](const ResourceAccess& access) {
                updateResourceState(access.index, access.usage);
            });
        }

        // if we have any barriers, submit them
        barriers.submit(mFrameSchedule, list);

        schedule.recordCommands(list, PassHandle{i});
    }

    // transition imported and created resources back to their initial state
    for (auto& [handle, state] : restore) {
        barriers.transition(handle, initial[handle], state);
    }

    barriers.submit(mFrameSchedule, list);

    // submit the last command list
    schedule.submitCommandBuffer(list);
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
                ID3D12GraphicsCommandList1 *commands = getCommandList(barrier.handle);
                commands->ResourceBarrier(barrier.size(), barrier.data());
            },
            [&](events::OpenCommands open) {
                gRenderLog.info("OpenCommands {}.{}", mFrameData[open.handle.index].type, open.handle.index);
                resetCommandBuffer(open.handle);
            },
            [&](events::RecordCommands record) {
                gRenderLog.info("RecordCommands {}.{}", mRenderPasses[record.pass.index].name, record.pass.index);
                ID3D12GraphicsCommandList1 *commands = getCommandList(record.handle);
                auto& pass = mRenderPasses[record.pass.index];
                RenderContext ctx{getContext(), *this, pass, commands};

                PIXBeginEvent(commands, PIX_COLOR_INDEX(colour++), "%s", pass.name.c_str());
                pass.execute(ctx);
                PIXEndEvent(commands);
            },
            [&](events::SubmitCommands submit) {
                gRenderLog.info("SubmitCommands {}.{}", mFrameData[submit.handle.index].type, submit.handle.index);
                closeCommandBuffer(submit.handle);
                ID3D12GraphicsCommandList1 *commands = getCommandList(submit.handle);
                ID3D12CommandQueue *queue = mContext.getQueue(getCommandListType(submit.handle));

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
