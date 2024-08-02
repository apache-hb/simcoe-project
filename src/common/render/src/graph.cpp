#include "stdafx.hpp"

#include "render/graph.hpp"

#include "render/render.hpp"
#include "render/format.hpp"

using namespace sm;
using namespace sm::graph;

using enum render::ResourceState::Inner;

using PassBuilder = FrameGraph::PassBuilder;
using AccessBuilder = FrameGraph::AccessBuilder;

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

#if 0
static bool isReadState(D3D12_RESOURCE_STATES states) {
    return (states & ~D3D12_RESOURCE_STATE_GENERIC_READ) == 0;
}

static bool isShaderResourceState(D3D12_RESOURCE_STATES states) {
    return (states & ~D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE) == 0;
}

static bool isWriteState(D3D12_RESOURCE_STATES states) {
    static constexpr D3D12_RESOURCE_STATES kWriteStates
        = D3D12_RESOURCE_STATE_RENDER_TARGET
        | D3D12_RESOURCE_STATE_UNORDERED_ACCESS
        | D3D12_RESOURCE_STATE_DEPTH_WRITE
        | D3D12_RESOURCE_STATE_STREAM_OUT
        | D3D12_RESOURCE_STATE_COPY_DEST;

    return (states & ~kWriteStates) == 0;
}
#endif
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

AccessBuilder PassBuilder::read(Handle handle, sm::StringView name, Usage access) {
    ResourceAccess& self = mRenderPass.reads.emplace_back(ResourceAccess{sm::String{name}, handle, access});

    return AccessBuilder{mFrameGraph, self, handle};
}

AccessBuilder PassBuilder::write(Handle handle, sm::StringView name, Usage access) {
    if (mFrameGraph.is_imported(handle)) {
        hasSideEffects(true);
    }

    ResourceAccess& self = addWriteAccess(handle, name, access);

    return AccessBuilder{mFrameGraph, self, handle};
}

AccessBuilder PassBuilder::create(ResourceInfo info, sm::StringView name, Usage access) {
    auto id = fmt::format("{}/{}", mRenderPass.name, name);
    Handle handle = mFrameGraph.add_transient(info, access, id);
    ResourceAccess& self = mRenderPass.creates.emplace_back(ResourceAccess{id, handle, access});
    addWriteAccess(handle, name, access);
    return AccessBuilder{mFrameGraph, self, handle};
}

AccessBuilder PassBuilder::read(Handle handle, sm::StringView name) {
    ResourceAccess& self = mRenderPass.reads.emplace_back(ResourceAccess{sm::String{name}, handle, Usage::eManualOverride});

    return AccessBuilder{mFrameGraph, self, handle};
}

PassBuilder& PassBuilder::hasSideEffects(bool effects) {
    mRenderPass.has_side_effects = effects;
    return *this;
}

bool FrameGraph::is_imported(Handle handle) const {
    return mHandles[handle.index].type == ResourceType::eImported;
}

uint FrameGraph::add_handle(ResourceHandle handle, Usage access) {
    uint index = mHandles.size();
    mHandles.emplace_back(std::move(handle));
    return index;
}

RenderPassHandle FrameGraph::findNextHandleUse(size_t start, Handle handle) const {
    if (start >= mRenderPasses.size()) {
        return RenderPassHandle{UINT_MAX};
    }

    for (uint i = start + 1; i < mRenderPasses.size(); i++) {
        auto& pass = mRenderPasses[i];
        if (!pass.is_used())
            continue;

        if (pass.uses_handle(handle)) {
            return RenderPassHandle{i};
        }
    }

    return RenderPassHandle{UINT_MAX};
}

Usage FrameGraph::getNextAccess(size_t start, Handle handle) const {
    auto next = findNextHandleUse(start, handle);
    if (!next.is_valid()) {
        return Usage::eUnknown;
    }

    auto& pass = mRenderPasses[next.index];
    return pass.get_handle_usage(handle);
}

Handle FrameGraph::add_transient(ResourceInfo info, Usage access, sm::StringView name) {
    ResourceHandle handle = {
        .name = sm::String{name},
        .info = info,
        .type = ResourceType::eTransient,
        .access = access,
    };

    return {add_handle(handle, access)};
}

Handle FrameGraph::include(sm::StringView name, ResourceInfo info, Usage access, ID3D12Resource *resource) {
    ResourceHandle handle = {
        .name = sm::String{name},
        .info = info,
        .type = resource ? ResourceType::eManaged : ResourceType::eImported,
        .access = access,
        .external = resource
    };

    uint index = add_handle(handle, access);
    return {index};
}

void FrameGraph::update(Handle handle, ID3D12Resource *resource) {
    mHandles[handle.index].external = resource;
}

void FrameGraph::update_rtv(Handle handle, render::RtvIndex rtv) {
    mHandles[handle.index].descriptors.rtv = rtv;
}

void FrameGraph::update_dsv(Handle handle, render::DsvIndex dsv) {
    mHandles[handle.index].descriptors.dsv = dsv;
}

void FrameGraph::update_srv(Handle handle, render::SrvIndex srv) {
    mHandles[handle.index].descriptors.srv = srv;
}

void FrameGraph::update_uav(Handle handle, render::SrvIndex uav) {
    mHandles[handle.index].descriptors.uav = uav;
}

FrameGraph::ResourceData& FrameGraph::getResourceData(Handle handle) {
    auto& info = mHandles[handle.index];
    auto all = getAllResourceData(handle);
    return info.isBuffered() ? all[mFrameIndex] : all.front();
}

sm::Span<FrameGraph::ResourceData> FrameGraph::getAllResourceData(Handle handle) {
    auto& info = mHandles[handle.index];
    return info.resources.viewOfData(mResources.data());
}

DescriptorPack& FrameGraph::getResourceDescriptors(Handle handle) {
    auto& info = mHandles[handle.index];
    if (info.isExternal())
        return info.descriptors;

    return getResourceData(handle).getDescriptors();
}

ID3D12Resource *FrameGraph::resource(Handle handle) {
    auto& info = mHandles[handle.index];
    if (info.isExternal())
        return info.external;

    return getResourceData(handle).getResource();
}

render::RtvIndex FrameGraph::rtv(Handle handle) {
    return getResourceDescriptors(handle).rtv;
}

render::DsvIndex FrameGraph::dsv(Handle handle) {
    return getResourceDescriptors(handle).dsv;
}

render::SrvIndex FrameGraph::srv(Handle handle) {
    return getResourceDescriptors(handle).srv;
}

render::SrvIndex FrameGraph::uav(Handle handle) {
    return getResourceDescriptors(handle).uav;
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

AccessBuilder& AccessBuilder::withStates(D3D12_RESOURCE_STATES value) {
    mAccess.states = value;
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
        auto& state = states[resource];
        switch (usage) {
        case ePixelShaderResource:
        case eTextureRead:
            state.srv = true;
            break;

        case eTextureWrite:
        case eBufferRead:
        case eBufferWrite:
            state.uav = true;
            break;

        case eRenderTarget:
            state.rtv = true;
            break;

        case eDepthRead:
        case eDepthWrite:
            state.dsv = true;
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

        CTASSERTF(states.contains(resource), "resource %u not found in usage tracker", resource);

        UsageBits usage = states.at(resource);
        if (usage.uav)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        if (usage.dsv)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        if (usage.rtv)
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        if (!usage.srv && !usage.uav && usage.dsv)
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;

        return flags;
    }
};

static D3D12_UNORDERED_ACCESS_VIEW_DESC buildUavDesc(const ResourceInfo& info) {
    DXGI_FORMAT format = info.getFormat();
    auto *array = info.asArray();
    CTASSERTF(array != nullptr, "cannot build uav desc for non-array resources currently");

    bool isStructuredBuffer = format == DXGI_FORMAT_UNKNOWN;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {
        .Format = format,
        .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
        .Buffer = {
            .FirstElement = 0,
            .NumElements = array->length,
            .StructureByteStride = isStructuredBuffer ? array->stride : 0,
        }
    };

    return uavDesc;
}

static D3D12_SHADER_RESOURCE_VIEW_DESC buildSrvDesc(const ResourceInfo& info) {
    DXGI_FORMAT format = info.getFormat();
    auto *array = info.asArray();
    CTASSERTF(array != nullptr, "cannot build srv desc for non-array resources currently");

    bool isStructuredBuffer = format == DXGI_FORMAT_UNKNOWN;
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
        .Format = format,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer = {
            .FirstElement = 0,
            .NumElements = array->length,
            .StructureByteStride = isStructuredBuffer ? array->stride : 0,
        }
    };

    return srvDesc;
}

void FrameGraph::createManagedResources() {
    mResources.clear();
    UsageTracker tracker;

    auto createNewResource = [&](const ResourceInfo& info, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state) {
        render::Resource resource;

        auto clear = getClearValue(info.getClearValue());
        D3D12_CLEAR_VALUE *ptr = clear.Format == DXGI_FORMAT_UNKNOWN ? nullptr : &clear;

        if (auto *array = info.asArray()) {
            SM_ASSERT_HR(mContext.createBufferResource(resource, D3D12_HEAP_TYPE_DEFAULT, array->getSize(), D3D12_RESOURCE_STATE_COMMON, flags));
        } else if (auto *tex2d = info.asTex2d()) {
            auto [width, height] = tex2d->size;
            auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                /*format=*/ info.getFormat(),
                /*width=*/ width,
                /*height=*/ height,
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

    auto createResourceViews = [&](uint i, const ResourceHandle& handle, ID3D12Resource *resource) -> DescriptorPack {
        ID3D12Device *device = mContext.getDevice();
        DescriptorPack pack;
        if (tracker.needsSrv(i)) {
            auto srv = mContext.mSrvPool.allocate();

            const auto srvHandle = mContext.mSrvPool.cpu_handle(srv);
            if (handle.info.isArray()) {
                const D3D12_SHADER_RESOURCE_VIEW_DESC desc
                    = handle.srv_desc.has_value()
                    ? handle.srv_desc.value()
                    : buildSrvDesc(handle.info);

                device->CreateShaderResourceView(resource, &desc, srvHandle);
            } else {
                const D3D12_SHADER_RESOURCE_VIEW_DESC *desc = handle.srv_desc ? &*handle.srv_desc : nullptr;
                device->CreateShaderResourceView(resource, desc, srvHandle);
            }

            pack.srv = srv;
        }

        if (tracker.needsUav(i)) {
            auto uav = mContext.mSrvPool.allocate();

            const auto uavHandle = mContext.mSrvPool.cpu_handle(uav);
            const D3D12_UNORDERED_ACCESS_VIEW_DESC desc
                = handle.uav_desc.has_value()
                ? handle.uav_desc.value()
                : buildUavDesc(handle.info);

            device->CreateUnorderedAccessView(resource, nullptr, &desc, uavHandle);

            pack.uav = uav;
        }

        if (tracker.needsDsv(i)) {
            auto dsv = mContext.mDsvPool.allocate();

            const auto dsvHandle = mContext.mDsvPool.cpu_handle(dsv);
            const D3D12_DEPTH_STENCIL_VIEW_DESC *desc = handle.dsv_desc ? &*handle.dsv_desc : nullptr;
            device->CreateDepthStencilView(resource, desc, dsvHandle);

            pack.dsv = dsv;
        }

        if (tracker.needsRtv(i)) {
            auto rtv = mContext.mRtvPool.allocate();

            const auto rtvHandle = mContext.mRtvPool.cpu_handle(rtv);
            const D3D12_RENDER_TARGET_VIEW_DESC *desc = handle.rtv_desc ? &*handle.rtv_desc : nullptr;
            device->CreateRenderTargetView(resource, desc, rtvHandle);

            pack.rtv = rtv;
        }

        return pack;
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
        D3D12_RESOURCE_STATES state = getStateFromUsage(handle.access);

        uint first = mResources.size();

        if (handle.isBuffered()) {
            uint count = mContext.getSwapChainLength();
            for (uint i = 0; i < count; i++) {
                auto resource = createNewResource(handle.info, flags, state);
                resource.rename(fmt::format("{}/buffer ({}/{})", handle.name, i + 1, count));

                mResources.emplace_back(std::move(resource));
            }
        } else {
            auto resource = createNewResource(handle.info, flags, state);
            resource.rename(handle.name);

            mResources.emplace_back(std::move(resource));
        }

        uint last = mResources.size();

        handle.resources = { first, last };
    }

    for (uint index = 0; index < mHandles.size(); index++) {
        auto& handle = mHandles[index];
        if (!handle.is_used() || handle.is_imported()) continue;

        auto data = getAllResourceData(Handle{index});
        for (auto& buffer : data) {
            buffer.descriptors = createResourceViews(index, handle, buffer.getResource());
        }
    }
}

///
/// per frame command data
///

void FrameGraph::FrameCommandData::resize(ID3D12Device1 *device, uint length) {
    for (uint i = 0; i < allocators.size(); i++) {
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

CommandListHandle FrameGraph::addCommandList(render::CommandListType type) {
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

// test if the resource state is valid for a transition barrier
// on the given queue type
#if 0
static bool validQueueState(D3D12_COMMAND_LIST_TYPE type, D3D12_RESOURCE_STATES state) {
    switch (type) {
    case D3D12_COMMAND_LIST_TYPE_COPY:
        return state == D3D12_RESOURCE_STATE_COMMON
            || state == D3D12_RESOURCE_STATE_COPY_SOURCE
            || state == D3D12_RESOURCE_STATE_COPY_DEST;

    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        return state == D3D12_RESOURCE_STATE_COMMON
            || state == D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
            || state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        return state == D3D12_RESOURCE_STATE_COMMON
            || state == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
            || state == D3D12_RESOURCE_STATE_RENDER_TARGET
            || state == D3D12_RESOURCE_STATE_DEPTH_READ
            || state == D3D12_RESOURCE_STATE_DEPTH_WRITE
            || state == D3D12_RESOURCE_STATE_INDEX_BUFFER
            || state == D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;

    default:
        return false;
    }
}
#endif

void FrameGraph::schedule_graph() {
    // responsible for adding new events to the frame schedule
    struct ScheduleBuilder {
        FrameGraph& self;
        FrameSchedule& schedule;

        CommandListHandle openCommandBuffer(render::CommandListType type) {
            CommandListHandle handle = self.addCommandList(type);

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
            if (signal == wait) return;

            events::DeviceSync sync = {
                .signal = signal,
                .wait = wait,
                .fence = self.addFence(fmt::format("GraphFence(signal: {}, wait: {})", signal, wait))
            };

            schedule.push_back(sync);
        }
    } schedule = { *this, mFrameSchedule };

    // track the expected state of each resource
    sm::HashMap<Handle, D3D12_RESOURCE_STATES> current;
    struct BarrierRecord {
        FrameGraph& graph;

        sm::Vector<events::Transition> transitions;
        sm::Vector<events::UnorderedAccess> uavs;

        BarrierRecord(FrameGraph& graph)
            : graph(graph)
        { }

        void transition(Handle handle, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
            if (before == after) return;

            // if the new state is a subset of the old state
            // and its read only, we should skip the transition
            if (isReadOnlyState(before | after)) {
                if ((after | before) == before)
                    return;
            }

            transitions.push_back(events::Transition{handle, before, after});
        }

        void uav(Handle handle) {
            ID3D12Resource *resource = graph.resource(handle);
            CTASSERTF(resource != nullptr, "Resource %u is null", handle.index);

            uavs.push_back(events::UnorderedAccess{handle});
        }

        void submit(FrameSchedule& schedule, CommandListHandle handle) {
            if (transitions.empty() && uavs.empty()) return;

            events::ResourceBarrier event{handle, transitions, uavs};
            transitions.clear();
            uavs.clear();

            schedule.push_back(event);
        }
    } barriers = { *this };

    auto updateResourceFlags = [&](Handle index, D3D12_RESOURCE_STATES states) {
        if (current.contains(index)) {
            barriers.transition(index, current[index], states);
        }

        current[index] = states;
    };

    auto updateResourceState = [&](Handle index, Usage access) {
        updateResourceFlags(index, getStateFromUsage(access));
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

        auto state = getStateFromUsage(handle.access);
        restore.push_back({idx, state});
        current[idx] = state;
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
    std::set<RenderPassHandle> hazards;

    for (uint i = 0; i < mRenderPasses.size(); i++) {
        auto& pass = mRenderPasses[i];
        if (!pass.is_used()) continue;

        // if the queue type changes, add a sync event
        if (pass.type != getCommandListType(list)) {
            // submit the last command lists work
            schedule.submitCommandBuffer(list);

            // syncronize with all the queues this new pass depends on
            for (auto it = hazards.begin(); it != hazards.end(); ) {
                RenderPass& dep = mRenderPasses[(*it).index];

                if (pass.depends_on(dep) && pass.type != dep.type) {
                    D3D12_COMMAND_LIST_TYPE ty = dep.type.as_facade();

                    if (!queueSyncStatus.isQueueSynced(ty)) {
                        schedule.syncQueue(dep.type, pass.type);
                        queueSyncStatus.markQueueSynced(ty);
                        it = hazards.erase(it);
                        continue;
                    }
                }

                ++it;
            }

            queueSyncStatus.reset();

            // open a new command buffer
            list = schedule.openCommandBuffer(pass.type);
        }

        hazards.insert(RenderPassHandle{i});

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
        for (const ResourceAccess& access : pass.creates) {
            D3D12_RESOURCE_STATES state = getStateFromUsage(access.usage);
            restore.push_back({access.index, state});
            current[access.index] = state;
        }

        // dont do any resource transitions on the copy queue
        // cant be bothered to try and figure out the edge cases
        auto type = getCommandListType(list);
        if (type == render::CommandListType::eCopy) {
            schedule.recordCommands(list, PassHandle{i});

            // TODO: bad assumptions?
            pass.foreach(eRead | eWrite, [&](const ResourceAccess& access) {
                updateResourceFlags(access.index, access.getStateFlags());
            });

            continue;
        }

        pass.foreach(eRead | eWrite, [&](const ResourceAccess& access) {
            updateResourceFlags(access.index, access.getStateFlags());
        });

        barriers.submit(mFrameSchedule, list);

        schedule.recordCommands(list, PassHandle{i});

        if (type == render::CommandListType::eDirect) {
            pass.foreach(eRead | eWrite, [&](const ResourceAccess& access) {

                // collect all the future states of this resource if its only read
                D3D12_RESOURCE_STATES mask = D3D12_RESOURCE_STATE_COMMON;
                bool onlyReadStates = forEachAccess(i, access.index, [&](const ResourceAccess& next) {
                    if (!next.isReadState())
                        return false;

                    mask |= next.getStateFlags();
                    return true;
                });

                if (onlyReadStates) {
                    updateResourceFlags(access.index, mask);
                } else if (auto next = getNextAccess(i, access.index); next != Usage::eUnknown) {
                    updateResourceState(access.index, next);
                }
            });

            barriers.submit(mFrameSchedule, list);
        }
    }

    // syncronize with all the queues the final pass depends on
    for (RenderPassHandle dep : hazards) {
        RenderPass& pass = mRenderPasses[dep.index];
        D3D12_COMMAND_LIST_TYPE ty = pass.type.as_facade();

        if (!queueSyncStatus.isQueueSynced(ty)) {
            schedule.syncQueue(pass.type, getCommandListType(list));
            queueSyncStatus.markQueueSynced(ty);
        }
    }

    // transition imported and created resources back to their initial state
    for (auto& [handle, state] : restore) {
        barriers.transition(handle, current[handle], state);
    }

    barriers.submit(mFrameSchedule, list);

    // submit the last command list
    schedule.submitCommandBuffer(list);
}

void DescriptorPack::release(render::IDeviceContext& context) {
    context.mSrvPool.safe_release(srv);
    context.mSrvPool.safe_release(uav);
    context.mDsvPool.safe_release(dsv);
    context.mRtvPool.safe_release(rtv);
}

void FrameGraph::destroyManagedResources() {
    for (auto& handle : mHandles) {
        if (handle.is_imported()) continue;

        handle.descriptors.release(mContext);
    }

    for (auto& [_, descriptors] : mResources) {
        descriptors.release(mContext);
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
#if SMC_RENDER_FRAMEGRAPH_TRACE
                gRenderLog.info("Sync(signal: {}, wait: {})", sync.signal, sync.wait);
#endif
                ID3D12CommandQueue *signal = mContext.getQueue(sync.signal);
                ID3D12CommandQueue *wait = mContext.getQueue(sync.wait);
                auto& [fence, value] = getFence(sync.fence);

                uint64 it = value++;

                SM_ASSERT_HR(signal->Signal(fence.get(), it));
                SM_ASSERT_HR(wait->Wait(fence.get(), it));
            },
            [&](events::ResourceBarrier& event) {
#if SMC_RENDER_FRAMEGRAPH_TRACE
                gRenderLog.info("ResourceBarrier({}) - {}", mFrameData[event.handle.index].type, event.handle.index);
#endif
                ID3D12GraphicsCommandList1 *commands = getCommandList(event.handle);
                event.build(*this);
                commands->ResourceBarrier(event.size(), event.data());
            },
            [&](events::OpenCommands open) {
#if SMC_RENDER_FRAMEGRAPH_TRACE
                auto& list = mFrameData[open.handle.index];
                gRenderLog.info("OpenCommands({}) - {}", list.type, open.handle.index);
#endif

                resetCommandBuffer(open.handle);
            },
            [&](events::RecordCommands record) {
                auto& pass = mRenderPasses[record.pass.index];
#if SMC_RENDER_FRAMEGRAPH_TRACE
                gRenderLog.info("RecordCommands({}) - {}", pass.name, record.pass.index);
                for (auto& access : pass.reads) {
                    gRenderLog.info("  | read {} ({})", access.name, getStateFromUsage(access.usage));
                }

                for (auto& access : pass.writes) {
                    gRenderLog.info("  | write {} ({})", access.name, getStateFromUsage(access.usage));
                }

                for (auto& access : pass.creates) {
                    gRenderLog.info("  | create {} ({})", access.name, getStateFromUsage(access.usage));
                }
#endif

                ID3D12GraphicsCommandList1 *commands = getCommandList(record.handle);
                RenderContext ctx{getContext(), *this, pass, commands};

                PIXBeginEvent(commands, PIX_COLOR_INDEX(colour++), "%s", pass.name.c_str());
                pass.execute(ctx);
                PIXEndEvent(commands);
            },
            [&](events::SubmitCommands submit) {
                auto& list = mFrameData[submit.handle.index];

#if SMC_RENDER_FRAMEGRAPH_TRACE
                gRenderLog.info("SubmitCommands({}) - {}", list.type, submit.handle.index);
#endif

                closeCommandBuffer(submit.handle);
                ID3D12CommandQueue *queue = mContext.getQueue(list.type);

                ID3D12CommandList *lists[] = { list.commands.get() };
                queue->ExecuteCommandLists(1, lists);
            }
        }, step);
    }
}
