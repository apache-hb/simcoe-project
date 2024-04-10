#include "stdafx.hpp"

#include "render/graph.hpp"

#include "render/render.hpp"

#include "core/set.hpp"

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

void PassBuilder::add_write(Handle handle, sm::StringView name, Usage access) {
    mRenderPass.writes.push_back({sm::String{name}, handle, access});
}

void PassBuilder::read(Handle handle, sm::StringView name, Usage access) {
    mRenderPass.reads.push_back({sm::String{name}, handle, access});
}

void PassBuilder::write(Handle handle, sm::StringView name, Usage access) {
    if (mFrameGraph.is_imported(handle)) {
        side_effects(true);
    }

    add_write(handle, name, access);
}

Handle PassBuilder::create(ResourceInfo info, sm::StringView name, Usage access) {
    auto id = fmt::format("{}/{}", mRenderPass.name, name);
    Handle handle = mFrameGraph.add_transient(info, access, id);
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

static constexpr D3D12_RESOURCE_STATES get_usage_state(Usage usage) {
    using enum Usage::Inner;
    switch (usage) {
    case ePresent: return D3D12_RESOURCE_STATE_PRESENT;
    case eUnknown: return D3D12_RESOURCE_STATE_COMMON;

    case eTextureRead: return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
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

static bool used_as_srv(Usage usage) {
    return (usage == Usage::eTextureRead);
}

static bool used_as_rtv(Usage usage) {
    return (usage == Usage::eRenderTarget);
}

static bool used_as_dsv(Usage usage) {
    return (usage == Usage::eDepthWrite || usage == Usage::eDepthRead);
}

static bool used_as_uav(Usage usage) {
    return (usage == Usage::eTextureWrite || usage == Usage::eBufferWrite || usage == Usage::eBufferRead);
}

static constexpr std::optional<D3D12_CLEAR_VALUE> get_clear_value(const Clear& info) {
    D3D12_CLEAR_VALUE storage;

    switch (info.type) {
    case Clear::eColour:
        storage.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        storage.Color[0] = info.colour.r;
        storage.Color[1] = info.colour.g;
        storage.Color[2] = info.colour.b;
        storage.Color[3] = info.colour.a;
        break;
    case Clear::eDepth:
        storage.Format = DXGI_FORMAT_D32_FLOAT;
        storage.DepthStencil.Depth = info.depth;
        storage.DepthStencil.Stencil = 0;
        break;
    case Clear::eEmpty:
        return std::nullopt;
    }

    return storage;
}

struct UsageTracker {
    sm::HashMap<uint, D3D12_RESOURCE_STATES> states;

    D3D12_RESOURCE_STATES at_or(uint resource, D3D12_RESOURCE_STATES state) const {
        if (states.contains(resource)) {
            return states.at(resource);
        }

        return state;
    }

    void access(uint resource, D3D12_RESOURCE_STATES state) {
        states[resource] |= state;
    }

    bool needs_uav(uint resource) const {
        return (at_or(resource, D3D12_RESOURCE_STATE_COMMON) & D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    }

    bool needs_srv(uint resource) const {
        return (at_or(resource, D3D12_RESOURCE_STATE_COMMON) & D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    bool needs_rtv(uint resource) const {
        return (at_or(resource, D3D12_RESOURCE_STATE_COMMON) & D3D12_RESOURCE_STATE_RENDER_TARGET);
    }

    bool needs_dsv(uint resource) const {
        return (at_or(resource, D3D12_RESOURCE_STATE_COMMON) & (D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ));
    }

    D3D12_RESOURCE_FLAGS get_resource_flags(uint resource) const {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        if (needs_uav(resource)) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        if (needs_rtv(resource)) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }

        if (needs_dsv(resource)) {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }

        if (!needs_srv(resource) && !needs_uav(resource)) {
            flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
        }

        return flags;
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
    UsageTracker tracker;

    for (uint i = 0; i < mHandles.size(); i++) {
        auto& handle = mHandles[i];
        if (!handle.is_used() || handle.is_imported()) continue;

        tracker.access(i, get_usage_state(handle.access));
    }

    for (auto& pass : mRenderPasses) {
        if (!pass.is_used()) continue;

        pass.foreach(eRead | eWrite | eCreate, [&](const ResourceAccess& access) {
            tracker.access(access.index.index, get_usage_state(access.usage));
        });
    }

    for (uint i = 0; i < mHandles.size(); i++) {
        // skip resources that we don't need to create
        auto& handle = mHandles[i];
        if (!handle.is_used() || handle.is_imported()) continue;

        const auto& info = handle.info;

        auto clear = get_clear_value(info.clear);
        D3D12_CLEAR_VALUE *ptr = clear ? &*clear : nullptr;

        const auto flags = tracker.get_resource_flags(i);
        auto desc = build_desc(info, flags);

        D3D12_RESOURCE_STATES state = get_usage_state(info.usage);
        auto& resource = mResources.emplace_back();
        SM_ASSERT_HR(mContext.create_resource(resource, D3D12_HEAP_TYPE_DEFAULT, desc, state, ptr));

        resource.rename(handle.name);

        handle.resource = resource.get();
    }
}

void FrameGraph::create_resource_descriptors() {
    auto& device = mContext.mDevice;

    for (auto& pass : mRenderPasses) {
        pass.foreach(eRead | eWrite | eCreate, [&](ResourceAccess& access) {
            auto& handle = mHandles[access.index.index];
            if (used_as_uav(access.usage)) {
                access.uav = mContext.mSrvPool.allocate();

                const auto srv_handle = mContext.mSrvPool.cpu_handle(access.uav);
                device->CreateUnorderedAccessView(handle.resource, nullptr, std::get_if<D3D12_UNORDERED_ACCESS_VIEW_DESC>(&access.view), srv_handle);
            }

            if (used_as_srv(access.usage)) {
                access.srv = mContext.mSrvPool.allocate();

                const auto srv_handle = mContext.mSrvPool.cpu_handle(access.srv);
                device->CreateShaderResourceView(handle.resource, std::get_if<D3D12_SHADER_RESOURCE_VIEW_DESC>(&access.view), srv_handle);
            }

            if (used_as_dsv(access.usage)) {
                access.dsv = mContext.mDsvPool.allocate();

                const auto dsv_handle = mContext.mDsvPool.cpu_handle(access.dsv);
                device->CreateDepthStencilView(handle.resource, std::get_if<D3D12_DEPTH_STENCIL_VIEW_DESC>(&access.view), dsv_handle);
            }

            if (used_as_rtv(access.usage)) {
                access.rtv = mContext.mRtvPool.allocate();

                const auto rtv_handle = mContext.mRtvPool.cpu_handle(access.rtv);
                device->CreateRenderTargetView(handle.resource, std::get_if<D3D12_RENDER_TARGET_VIEW_DESC>(&access.view), rtv_handle);
            }
        });
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
    if (type != render::CommandListType::eCopy) {
        ID3D12DescriptorHeap *heaps[] = { mContext.mSrvPool.get() };
        commands->SetDescriptorHeaps(1, heaps);
    }
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

        logs::gRender.info("Submit {} barriers", transitions.size());

        events::ResourceBarrier event{handle, transitions};
        schedule.push_back(event);

        transitions.clear();
    }
};

void FrameGraph::schedule_graph() {
    // track the expected state of each resource
    sm::HashMap<Handle, D3D12_RESOURCE_STATES> initial;
    BarrierRecord barriers;

    auto update_state = [&](Handle index, Usage access) {
        D3D12_RESOURCE_STATES state = get_usage_state(access);

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

        auto state = get_usage_state(handle.access);
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

    logs::gRender.info("begin command recording");

    // open the initial command list
    events::OpenCommands open = {
        .handle = cmd
    };

    mFrameSchedule.push_back(open);

    sm::Vector<RenderPass> stack;

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

            logs::gRender.info("Submit commands for {}", get_command_type(cmd));

            bool has_dependency = [&] {
                for (auto& p : stack) {
                    if (pass.depends_on(p)) return true;
                }

                return false;
            }();

            stack.clear();

            if (has_dependency) {
                // assume the next queue depends on the current one
                events::DeviceSync sync = {
                    .signal = get_command_type(cmd),
                    .wait = pass.type
                };

                mFrameSchedule.push_back(sync);

                logs::gRender.info("Sync {} -> {}", sync.signal, sync.wait);
            }

            cmd = add_commands(pass.type);

            // open the next command list
            events::OpenCommands open = {
                .handle = cmd
            };

            mFrameSchedule.push_back(open);

            stack.push_back(pass);
        }

        stack.push_back(pass);

        for (auto& access : pass.creates) {
            restore.push_back({access.index, get_usage_state(access.usage)});
            update_state(access.index, access.usage);
        }

        pass.foreach(eRead | eWrite, [&](const ResourceAccess& access) {
            update_state(access.index, access.usage);
        });

        // if we have any barriers, submit them
        barriers.submit(mFrameSchedule, cmd);

        // then record the commands for this pass
        events::RecordCommands record = {
            .handle = cmd,
            .pass = PassHandle{i}
        };

        mFrameSchedule.push_back(record);

        logs::gRender.info("Record commands for {}", pass.name);

        // only add early barriers on direct command list
        // not sure of all the special cases yet
        if (get_command_type(cmd) != render::CommandListType::eDirect)
            continue;

        logs::gRender.info("Searching for early barriers {}", pass.name);

        // if we can add barriers for the next pass, do so
        pass.foreach(eRead | eWrite | eCreate, [&](const auto& access) {
            auto n = find_next_use(i, access.index);
            logs::gRender.info("Next use of {} is {}", access.index.index, n.index);
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
    events::SubmitCommands submit = {
        .handle = cmd,
    };

    mFrameSchedule.push_back(submit);

    logs::gRender.info("finish command recording");
}

void FrameGraph::destroy_resources() {
    for (auto& handle : mHandles) {
        if (handle.is_imported()) continue;
    }

    for (auto& pass : mRenderPasses) {
        pass.foreach(eRead | eWrite | eCreate, [&](ResourceAccess& access) {
            if (is_imported(access.index)) return;

            mContext.mSrvPool.safe_release(access.srv);
            mContext.mSrvPool.safe_release(access.uav);
            mContext.mDsvPool.safe_release(access.dsv);
            mContext.mRtvPool.safe_release(access.rtv);
        });
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
