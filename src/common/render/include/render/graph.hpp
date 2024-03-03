#pragma once

#include "render/render.hpp"

#include <functional>

#include "graph.reflect.h" // IWYU pragma: export

/// list of stuff that we cant do yet
/// - branching on the graph, no coniditional passes or resources
/// - multiple passes that write to the same resource
///   - requires versioning of resources, dont have time to implement
namespace sm::graph {
    using namespace sm::math;

    using Access = render::ResourceState;

    struct Handle {
        uint index;
    };

    struct TextureInfo {
        sm::String name;
        uint2 size;
        render::Format format;
    };

    class FrameGraph {
        struct IGraphNode {
            uint refcount = 0;
        };

        struct ResourceAccess {
            Handle index;
            Access access;
        };

        struct RenderPass final : IGraphNode {
            sm::String name;
            bool has_side_effects = false;

            sm::SmallVector<ResourceAccess, 4> creates;
            sm::SmallVector<ResourceAccess, 4> reads;
            sm::SmallVector<ResourceAccess, 4> writes;

            std::function<void(FrameGraph&, render::Context&)> execute;

            bool should_execute() const { return has_side_effects || refcount > 0; }
        };

        struct ResourceHandle final : IGraphNode {
            TextureInfo info;
            ResourceType type;
            Access access;
            RenderPass *producer;
            RenderPass *last;

            ID3D12Resource *resource;

            // will be filled depending on the type of resource
            render::RtvIndex rtv = render::RtvIndex::eInvalid;
            render::DsvIndex dsv = render::DsvIndex::eInvalid;
            render::SrvIndex srv = render::SrvIndex::eInvalid;
        };

        render::Context& mContext;

        sm::Vector<RenderPass> mRenderPasses;
        sm::Vector<ResourceHandle> mHandles;
        sm::Vector<render::Resource> mResources;

        uint add_handle(ResourceHandle handle, Access access);
        Handle texture(TextureInfo info, Access access);

        bool is_imported(Handle handle) const;

    public:
        FrameGraph(render::Context& context)
            : mContext(context)
        { }

        class PassBuilder {
            FrameGraph& mFrameGraph;
            RenderPass& mRenderPass;

        public:
            PassBuilder(FrameGraph& graph, RenderPass& pass)
                : mFrameGraph(graph)
                , mRenderPass(pass)
            { }

            void read(Handle handle, Access access);
            void write(Handle handle, Access access);
            Handle create(TextureInfo info, Access access);

            void side_effects(bool effects);

            template<typename F>
            PassBuilder& execute(F&& execute) {
                mRenderPass.execute = std::forward<F>(execute);
                return *this;
            }
        };

        ID3D12Resource *resource(Handle handle);
        render::RtvIndex rtv(Handle handle);
        render::DsvIndex dsv(Handle handle);
        render::SrvIndex srv(Handle handle);

        PassBuilder pass(sm::StringView name);
        Handle include(sm::StringView name, TextureInfo info, Access access, ID3D12Resource *resource);

        void compile();
        void execute();
    };
}
