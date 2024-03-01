#pragma once

#include "render/render.hpp" // IWYU pragma: export

#include "graph.reflect.h" // IWYU pragma: export

namespace sm::graph {
    class IRenderPass;

    class PassBuilder;
    class FrameGraph;
    class Handle; // resource handle

    using ResourceState = D3D12_RESOURCE_STATES;
    using Barrier = D3D12_RESOURCE_BARRIER;

    struct Resource {
        render::Resource resource;

        render::DescriptorIndex rtv;
        render::DescriptorIndex dsv;
        render::DescriptorIndex srv;
        // render::DescriptorIndex uav;
    };

    struct HandleInfo {
        HandleType type;
        sm::String name;
        ResourceState state;
        render::Format format;

        union {
            struct Create {
                uint width; // 1 for full resolution, 2 for half
                uint height; // 1 for full resolution, 2 for half
                uint temporal; // number of frames to keep the resource
            } create;
        };
    };

    class Handle {
        ResourceType mType;
        Resource *mResource;

    public:
        Handle(ResourceType type, Resource *resource = nullptr)
            : mType(type)
            , mResource(resource)
        { }

        ResourceType get_type() const { return mType; }
        bool is_imported() const { return mType == ResourceType::eImported; }
        bool is_transient() const { return mType == ResourceType::eTransient; }

        Resource *get() const { return mResource; }
        void set_resource(Resource *resource) { mResource = resource; }
    };

    class PassBuilder {
        render::Context& mContext;
        IRenderPass& mPass;
        FrameGraph& mGraph;

    public:
        PassBuilder(render::Context& context, IRenderPass& pass, FrameGraph& graph)
            : mContext(context)
            , mPass(pass)
            , mGraph(graph)
        { }

        Handle read(sm::StringView name, ResourceState state);
        Handle write(sm::StringView name, ResourceState state);
        Handle create(sm::StringView name, ResourceState state, uint width, uint height, render::Format format);

        render::Context& get_context() { return mContext; }
        FrameGraph& get_graph() { return mGraph; }
    };

    class IRenderPass {
    protected:
        render::Context& ctx;
        FrameGraph& graph;

    public:
        IRenderPass(render::Context& context, FrameGraph& graph)
            : ctx(context)
            , graph(graph)
        { }

        virtual void create() { }
        virtual void destroy() { }

        // TODO: this two step construction is a little annoying
        virtual void build(PassBuilder& builder) = 0;
        virtual void execute() = 0;
    };

    struct ResourceInfo {
        sm::String name;
        HandleInfo info;
        Resource *resource;
    };

    class FrameGraph {
        sm::Vector<IRenderPass*> mPasses;
        sm::Vector<ResourceInfo> mResources;

        render::Context& mContext;

        void build_pass(sm::StringView name, IRenderPass *pass);

        Handle new_resource(ResourceType type, const HandleInfo& info, Resource *resource);

    public:
        FrameGraph(render::Context& context)
            : mContext(context)
        { }

        Handle create_resource(const HandleInfo& info);
        Handle import_resource(const HandleInfo& info, Resource *resource);

        template<std::derived_from<IRenderPass> T, typename... A> requires std::constructible_from<T, A&&...>
        void add_pass(sm::StringView name, A&&... args) {
            IRenderPass *pass = new T{std::forward<A>(args)...};
            build_pass(name, pass);
        }

        void build();
        void execute();
    };
}
