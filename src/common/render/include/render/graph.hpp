#pragma once

#include "core/string.hpp"
#include "core/vector.hpp"
#include "core/typeindex.hpp"
#include "core/map.hpp"

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

    struct ResourceSize {
        static ResourceSize tex2d(uint2 size);
        static ResourceSize buffer(uint size);

        enum { eTex2D, eBuffer } type;
        union {
            uint2 tex2d_size;
            uint buffer_size;
        };
    };

    struct ResourceInfo {
        ResourceSize sz;
        uint2 size;
        render::Format format;
        Clear clear;
    };

    struct IDeviceData {
        virtual ~IDeviceData() = default;

        virtual void setup(render::Context& context) = 0;
        virtual bool has_type(uint32 index) const = 0;
    };

    class FrameGraph {
        struct IGraphNode {
            uint refcount = 0;
        };

        struct ResourceAccess {
            sm::String name;
            Handle index;
            Access access;
        };

        struct RenderPass final : IGraphNode {
            enum Type { eDirect, eCompute, eCopy } type;

            sm::String name;
            IDeviceData *data;
            bool has_side_effects = false;

            sm::SmallVector<ResourceAccess, 4> creates;
            sm::SmallVector<ResourceAccess, 4> reads;
            sm::SmallVector<ResourceAccess, 4> writes;

            std::function<void(FrameGraph&)> execute;

            bool is_used() const { return has_side_effects || refcount > 0; }
        };

        struct ResourceHandle final : IGraphNode {
            // the name of the resource
            sm::String name;

            // info to create the resource
            ResourceInfo info;

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
            render::SrvIndex uav = render::SrvIndex::eInvalid;

            bool is_imported() const { return type == ResourceType::eImported; }
            bool is_managed() const { return type == ResourceType::eManaged || type == ResourceType::eTransient; }
            bool is_used() const { return is_managed() || refcount > 0; }
        };

        render::Context& mContext;

        sm::Vector<RenderPass> mRenderPasses;
        sm::Vector<ResourceHandle> mHandles;
        sm::Vector<render::Resource> mResources;

        sm::HashMap<uint32, sm::UniquePtr<IDeviceData>> mDeviceData;

        uint add_handle(ResourceHandle handle, Access access);
        Handle texture(ResourceInfo info, Access access, sm::StringView name);

        bool is_imported(Handle handle) const;

    public:
        void optimize();
        void create_resources();
        void destroy_resources();

        FrameGraph(render::Context& context)
            : mContext(context)
        { }

        class PassBuilder {
            FrameGraph& mFrameGraph;
            RenderPass& mRenderPass;

            void add_write(Handle handle, sm::StringView name, Access access);

        public:
            PassBuilder(FrameGraph& graph, RenderPass& pass)
                : mFrameGraph(graph)
                , mRenderPass(pass)
            { }

            void read(Handle handle, sm::StringView name, Access access);
            void write(Handle handle, sm::StringView name, Access access);
            Handle create(ResourceInfo info, sm::StringView name, Access access);

            void side_effects(bool effects);

            template<typename F> requires std::invocable<F, FrameGraph&>
            void bind(F&& execute) {
                mRenderPass.execute = execute;
            }
        };

        template<typename F> requires std::invocable<F, render::Context&>
        auto& device_data(F&& setup) {
            using ActualData = std::invoke_result_t<F, render::Context&>;
            struct DeviceData final : IDeviceData {
                F exec;
                ActualData data;

                void setup(render::Context& context) override {
                    data = std::move(exec(context));
                }

                bool has_type(uint32 index) const override {
                    return TypeIndex<ActualData>::index() == index;
                }

                DeviceData(F&& exec) : exec(std::move(exec)) { }
            };

            uint32 index = TypeIndex<ActualData>::index();
            if (auto it = mDeviceData.find(index); it != mDeviceData.end()) {
                return static_cast<DeviceData*>(it->second.get())->data;
            }

            auto [it, _] = mDeviceData.emplace(index, new DeviceData(std::forward<F>(setup)));

            it->second->setup(mContext);

            return static_cast<DeviceData*>(it->second.get())->data;
        }

        // update a handle with new data
        // this is quite dangerous, only really used for the swapchain
        void update(Handle handle, ID3D12Resource *resource);
        void update_rtv(Handle handle, render::RtvIndex rtv);
        void update_dsv(Handle handle, render::DsvIndex dsv);
        void update_srv(Handle handle, render::SrvIndex srv);
        void update_uav(Handle handle, render::SrvIndex uav);

        ID3D12Resource *resource(Handle handle);
        render::RtvIndex rtv(Handle handle);
        render::DsvIndex dsv(Handle handle);
        render::SrvIndex srv(Handle handle);
        render::SrvIndex uav(Handle handle);

        PassBuilder pass(sm::StringView name, RenderPass::Type type);

        PassBuilder graphics(sm::StringView name);
        PassBuilder compute(sm::StringView name);
        PassBuilder copy(sm::StringView name);

        Handle include(sm::StringView name, ResourceInfo info, Access access, ID3D12Resource *resource);

        render::Context& get_context() { return mContext; }

        void reset();
        void reset_device_data();
        void compile();
        void execute();
    };

    using PassBuilder = FrameGraph::PassBuilder;
}
