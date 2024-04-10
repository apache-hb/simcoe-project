#pragma once

#include "core/string.hpp"
#include "core/vector.hpp"
#include "core/typeindex.hpp"
#include "core/map.hpp"

#include "math/math.hpp"

#include "render/heap.hpp"
#include "render/resource.hpp"

#include <functional>

#include "render/graph/handle.hpp"
#include "render/graph/events.hpp"
#include "render/graph/render_pass.hpp"

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
    class FrameGraph;

    using namespace sm::math;

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
        static ResourceSize tex2d(uint2 size) {
            return { .type = eTex2D, .tex2d_size = size };
        }

        static ResourceSize buffer(uint size) {
            return { .type = eBuffer, .buffer_size = size };
        }

        enum { eTex2D, eBuffer } type;
        union {
            uint2 tex2d_size;
            uint buffer_size;
        };
    };

    struct ResourceInfo {
        ResourceSize sz;
        render::Format format;
        Clear clear;

        // specify the usage of this resource, in cases
        // where its created in a different state than its used
        Usage usage;
    };

    struct FrameCommandData {
        render::CommandListType type;
        sm::UniqueArray<render::Object<ID3D12CommandAllocator>> allocators;
        render::Object<ID3D12GraphicsCommandList1> commands;
    };

    using FrameSchedule = sm::Vector<events::Event>;

    class FrameGraph {
        struct RenderPassHandle {
            uint index;

            bool is_valid() const { return index != UINT_MAX; }
        };

        struct ResourceHandle final {
            uint refcount = 0;

            // the name of the resource
            sm::String name;

            // info to create the resource
            ResourceInfo info;

            // type of the resource (imported or transient)
            ResourceType type;

            // the initial state of the resource
            Usage access;

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

        FrameSchedule mFrameSchedule;


        ///
        /// frame data resources
        ///
        sm::Vector<FrameCommandData> mFrameData;

        void reset_frame_commands();
        void init_frame_commands();
        CommandListHandle add_commands(render::CommandListType type);
        render::CommandListType get_command_type(CommandListHandle handle);

        void open_commands(CommandListHandle handle);
        void close_commands(CommandListHandle handle);
        ID3D12GraphicsCommandList1 *get_commands(CommandListHandle handle);


        ///
        /// resource handles
        ///

        uint add_handle(ResourceHandle handle, Usage access);
        Handle add_transient(ResourceInfo info, Usage access, sm::StringView name);

        bool is_imported(Handle handle) const;

        RenderPassHandle find_next_use(size_t start, Handle handle) const {
            if (start >= mRenderPasses.size()) {
                return RenderPassHandle{UINT_MAX};
            }

            for (uint i = start + 1; i < mRenderPasses.size(); i++) {
                if (mRenderPasses[i].uses_handle(handle)) {
                    return RenderPassHandle{i};
                }
            }

            return RenderPassHandle{UINT_MAX};
        }

        void build_raw_events(events::ResourceBarrier& barrier);

        void create_resources();
        void create_resource_descriptors();

        // figure out what passes should be run on which queues
        // and when to insert sync points and barriers
        // has to be run after the graph is optimized
        void schedule_graph();

        // cull passes from the graph that are not used
        void optimize();

        void destroy_resources();
    public:

        FrameGraph(render::Context& context)
            : mContext(context)
        { }

        class PassBuilder {
            FrameGraph& mFrameGraph;
            RenderPass& mRenderPass;

            void add_write(Handle handle, sm::StringView name, Usage access, ResourceView view);

        public:
            PassBuilder(FrameGraph& graph, RenderPass& pass)
                : mFrameGraph(graph)
                , mRenderPass(pass)
            { }

            void read(Handle handle,         sm::StringView name, Usage access, ResourceView view = std::monostate{});
            void write(Handle handle,        sm::StringView name, Usage access, ResourceView view = std::monostate{});
            Handle create(ResourceInfo info, sm::StringView name, Usage access, ResourceView view = std::monostate{});

            void side_effects(bool effects);

            template<typename F> requires std::invocable<F, RenderContext&>
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

        PassBuilder pass(sm::StringView name, render::CommandListType type);

        PassBuilder graphics(sm::StringView name);
        PassBuilder compute(sm::StringView name);
        PassBuilder copy(sm::StringView name);

        Handle include(sm::StringView name, ResourceInfo info, Usage access, ID3D12Resource *resource);

        render::Context& get_context() { return mContext; }

        // destroy the render passes and resources
        void reset();

        // also destroy the device data
        void reset_device_data();
        void compile();
        void execute();
    };

    using PassBuilder = FrameGraph::PassBuilder;
}

template<>
struct std::hash<sm::graph::Handle> {
    size_t operator()(const sm::graph::Handle& handle) const {
        return std::hash<sm::uint>{}(handle.index);
    }
};
