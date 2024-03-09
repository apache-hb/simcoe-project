#pragma once

#include "core/text.hpp"
#include "core/vector.hpp"
#include "math/math.hpp"

#include "render/heap.hpp"
#include "render/resource.hpp"

#include <functional>

#include "render.reflect.h"
#include "graph.reflect.h"

namespace sm::render {
    struct Context;
}

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

    struct Clear {
        enum { eColour, eDepth, eEmpty } type;
        union {
            float4 colour;
            float depth;
        };

        Clear() : type(eEmpty) { }

        D3D12_CLEAR_VALUE *get_value(D3D12_CLEAR_VALUE& storage, render::Format format) const;
    };

    Clear clear_colour(float4 colour);

    Clear clear_depth(float depth);

    struct TextureInfo {
        sm::String name;
        uint2 size;
        render::Format format;
        Clear clear;
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

            bool is_used() const { return has_side_effects || refcount > 0; }
        };

        struct ResourceHandle final : IGraphNode {
            // info to create the resource
            TextureInfo info;

            // type of the resource (imported or transient)
            ResourceType type;

            // the initial state of the resource
            Access access;

            // the producer of the resource, null if it's imported
            RenderPass *producer = nullptr;

            // the last pass that uses the resource
            RenderPass *last = nullptr;

            // the resource itself
            ID3D12Resource *resource = nullptr;

            // will be filled depending on the type of resource
            render::RtvIndex rtv = render::RtvIndex::eInvalid;
            render::DsvIndex dsv = render::DsvIndex::eInvalid;
            render::SrvIndex srv = render::SrvIndex::eInvalid;

            bool is_imported() const { return type == ResourceType::eImported; }
            bool is_managed() const { return type == ResourceType::eManaged || type == ResourceType::eTransient; }
            bool is_used() const { return is_managed() || refcount > 0; }
        };

        render::Context& mContext;

        sm::Vector<RenderPass> mRenderPasses;
        sm::Vector<ResourceHandle> mHandles;
        sm::Vector<render::Resource> mResources;

        uint add_handle(ResourceHandle handle, Access access);
        Handle texture(TextureInfo info, Access access);

        bool is_imported(Handle handle) const;

        void optimize();
        void create_resources();
        void destroy_resources();

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
            void bind(F&& execute) {
                mRenderPass.execute = std::forward<F>(execute);
            }
        };

        // update a handle with new data
        // this is quite dangerous, only really used for the swapchain
        void update(Handle handle, ID3D12Resource *resource);
        void update(Handle handle, render::RtvIndex rtv);
        void update(Handle handle, render::DsvIndex dsv);
        void update(Handle handle, render::SrvIndex srv);

        ID3D12Resource *resource(Handle handle);
        render::RtvIndex rtv(Handle handle);
        render::DsvIndex dsv(Handle handle);
        render::SrvIndex srv(Handle handle);

        PassBuilder pass(sm::StringView name);
        Handle include(TextureInfo info, Access access, ID3D12Resource *resource);

        uint2 present_size() const;
        uint2 render_size() const;

        render::Context& get_context() { return mContext; }

        void reset();
        void compile();
        void execute();
    };

    using PassBuilder = FrameGraph::PassBuilder;
}
