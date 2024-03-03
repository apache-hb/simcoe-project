#include "render/graph.hpp"

#include "render/render.hpp"

#include "core/map.hpp"
#include "core/stack.hpp"

#include "d3dx12_core.h"

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
        side_effects(true);
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
    uint index = mHandles.size();
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

Handle FrameGraph::include(TextureInfo info, Access access, ID3D12Resource *resource) {
    ResourceHandle handle = {
        .info = info,
        .type = ResourceType::eImported,
        .access = access,
        .resource = resource
    };

    uint index = add_handle(handle, access);
    return {index};
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
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }

    return flags;
}

static constexpr D3D12_CLEAR_VALUE get_clear_value(Access access) {
    if (access.test(eRenderTarget)) {
        return { .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .Color = {0.0f, 0.0f, 0.0f, 1.0f} };
    } else if (access.test(eDepthTarget)) {
        return { .Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = {1.0f, 0} };
    }

    SM_NEVER("invalid access {}", access);
}

void FrameGraph::create_resources() {
    mResources.clear();

    auto& device = mContext.mDevice;

    for (auto& handle : mHandles) {
        // skip resources that are not used
        if (!handle.is_used()) continue;

        const auto& info = handle.info;

        if (!handle.is_imported()) {
            const auto flags = get_required_flags(handle.access);
            const auto clear = get_clear_value(handle.access);

            auto size = mContext.mSceneSize;
            size /= info.size;

            const auto desc = CD3DX12_RESOURCE_DESC::Tex2D(
                /*format=*/ info.format.as_facade(),
                /*width=*/ size.width,
                /*height=*/ size.height,
                /*arraySize=*/ 1,
                /*mipLevels=*/ 0,
                /*sampleCount=*/ 1,
                /*sampleQuality=*/ 0,
                /*flags=*/ flags
            );

            const auto state = handle.access.as_facade();

            auto& resource = mResources.emplace_back();
            SM_ASSERT_HR(mContext.create_resource(resource, D3D12_HEAP_TYPE_DEFAULT, desc, state, &clear));

            handle.resource = resource.get();
        }

        // TODO: this seems a little weird
        if (handle.access.test(eRenderTarget)) {
            mContext.mSink.info("RenderTarget created for {}", info.name);
            handle.rtv = mContext.mRtvPool.allocate();

            const auto rtv_handle = mContext.mRtvPool.cpu_handle(handle.rtv);
            device->CreateRenderTargetView(handle.resource, nullptr, rtv_handle);

            // create srv
            handle.srv = mContext.mSrvPool.allocate();
            const auto srv_handle = mContext.mSrvPool.cpu_handle(handle.srv);
            device->CreateShaderResourceView(handle.resource, nullptr, srv_handle);
        } else if (handle.access.test(eDepthTarget)) {
            mContext.mSink.info("DepthStencil created for {}", info.name);
            handle.dsv = mContext.mDsvPool.allocate();

            const auto dsv_handle = mContext.mDsvPool.cpu_handle(handle.dsv);
            device->CreateDepthStencilView(handle.resource, nullptr, dsv_handle);
        }
    }

    mContext.mSink.info("Resources created");
}

void FrameGraph::compile() {
    optimize();
    create_resources();
}

void FrameGraph::execute() {
    sm::HashMap<ID3D12Resource*, D3D12_RESOURCE_STATES> states;

    struct FinalState {
        ID3D12Resource *resource;
        D3D12_RESOURCE_STATES state;
    };
    sm::SmallVector<FinalState, 8> created_resources;

    auto& commands = mContext.mCommandList;
    for (auto& pass : mRenderPasses) {
        if (!pass.is_used()) continue;

        for (auto& [index, access] : pass.creates) {
            ID3D12Resource *resource = mHandles[index.index].resource;
            created_resources.push_back({resource, access.as_facade()});
            states[resource] = access.as_facade();
        }

        for (auto& [index, access] : pass.reads) {
            ID3D12Resource *resource = mHandles[index.index].resource;
            if (states.contains(resource)) commands.transition(resource, states[resource], access.as_facade());
            states[resource] = access.as_facade();
        }

        for (auto& [index, access] : pass.writes) {
            ID3D12Resource *resource = mHandles[index.index].resource;
            if (states.contains(resource)) commands.transition(resource, states[resource], access.as_facade());
            states[resource] = access.as_facade();
        }

        commands.submit_barriers();

        pass.execute(*this, mContext);
    }

    // transition created resource back to their initial state
    for (auto& [resource, state] : created_resources) {
        commands.transition(resource, states[resource], state);
    }

    commands.submit_barriers();
}
