#include "render/graph.hpp"

#include "core/map.hpp"
#include "core/stack.hpp"

#include "d3dx12_core.h"

using namespace sm;
using namespace sm::graph;

// Handle FrameGraph::new_resource(ResourceType type, const HandleInfo& info, Resource *resource) {

// }

Handle FrameGraph::create_resource(const HandleInfo& info) {
    SM_ASSERTF(info.type == HandleType::eCreate, "Invalid handle type {}", info.type);

    auto scene = mContext.mSceneSize;
    scene.width /= info.create.width;
    scene.height /= info.create.height;

    Resource resource;

    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    // if (info.state & D3D12_RESOURCE_STATE_RENDER_TARGET) {
    //     resource.rtv = mContext.mRtvAllocator.allocate();
    //     flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    // }

    // if (info.state & D3D12_RESOURCE_STATE_DEPTH_WRITE) {
    //     resource.dsv = mContext.mDsvAllocator.allocate();
    //     flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    // }

    if (info.state & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) {
        resource.srv = mContext.mSrvAllocator.allocate();
    }

    const auto kDesc = CD3DX12_RESOURCE_DESC::Tex2D(info.format.as_facade(), scene.width, scene.height, 1, 1, 1, 0, flags);

    D3D12_CLEAR_VALUE kClear = {
        .Format = info.format.as_facade(),
        .Color = { 0.0f, 0.0f, 0.0f, 1.0f },
    };

    if (info.state & D3D12_RESOURCE_STATE_DEPTH_WRITE) {
        kClear.DepthStencil = { 1.0f, 0 };
    }

    SM_ASSERT_HR(mContext.create_resource(resource.resource, D3D12_HEAP_TYPE_DEFAULT, kDesc, info.state, &kClear));

    CT_NEVER("Not implemented");
}

Handle FrameGraph::import_resource(const HandleInfo& info, Resource *resource) {
    return new_resource(ResourceType::eImported, info, resource);
}

void FrameGraph::build_pass(sm::StringView name, IRenderPass *pass) {
    PassBuilder builder{mContext, *pass, *this};
    pass->build(builder);
    mPasses.push_back(pass);
}
