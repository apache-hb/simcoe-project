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
    struct IDeviceContext;
}

/// list of stuff that we cant do yet
/// - branching on the graph, no coniditional passes or resources
/// - multiple passes that write to the same resource
///   - requires versioning of resources, dont have time to implement
namespace sm::graph {
    class FrameGraph;

    enum struct ClearType {
        eColour,
        eDepth,
        eEmpty
    };

    class Clear {
        ClearType mClearType;
        DXGI_FORMAT mFormat;

        union {
            math::float4 mClearColour;

            struct {
                float mClearDepth;
                uint8 mClearStencil;
            };
        };

    public:
        ClearType getClearType() const;
        DXGI_FORMAT getFormat() const;

        float getClearDepth() const;
        math::float4 getClearColour() const;

        static Clear empty();
        static Clear colour(math::float4 value, DXGI_FORMAT format);
        static Clear depthStencil(float depth, uint8 stencil, DXGI_FORMAT format);
    };

    struct ResourceSize {
        static ResourceSize tex2d(math::uint2 size) {
            return { .type = eTex2D, .tex2d_size = size };
        }

        static ResourceSize buffer(uint size) {
            return { .type = eBuffer, .buffer_size = size };
        }

        enum { eTex2D, eBuffer } type;
        union {
            math::uint2 tex2d_size;
            uint buffer_size;
        };
    };

    struct ResourceInfo {
        ResourceSize size;
        render::Format format;
        Clear clear;
    };

    class FrameGraph {
        struct FrameCommandData {
            render::CommandListType type;
            sm::UniqueArray<render::Object<ID3D12CommandAllocator>> allocators;
            render::Object<ID3D12GraphicsCommandList1> commands;

            void resize(ID3D12Device1 *device, uint length);
        };

        struct FenceData {
            render::Object<ID3D12Fence> fence;
            uint64 value = 1;
        };

        using FrameSchedule = sm::Vector<events::Event>;

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

            std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srv_desc;
            std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uav_desc;
            std::optional<D3D12_RENDER_TARGET_VIEW_DESC> rtv_desc;
            std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> dsv_desc;

            bool is_imported() const { return type == ResourceType::eImported; }
            bool is_managed() const { return type == ResourceType::eManaged || type == ResourceType::eTransient; }
            bool is_used() const { return is_managed() || refcount > 0; }
        };

        render::IDeviceContext& mContext;

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
        render::CommandListType getCommandListType(CommandListHandle handle);

        void resetCommandBuffer(CommandListHandle handle);
        void closeCommandBuffer(CommandListHandle handle);
        ID3D12GraphicsCommandList1 *getCommandList(CommandListHandle handle);

        ///
        /// queue fences
        ///

        sm::Vector<FenceData> mFences;

        FenceHandle addFence(sm::StringView name);
        FenceData& getFence(FenceHandle handle);

        void clearFences();

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

        void createManagedResources();

        // figure out what passes should be run on which queues
        // and when to insert sync points and barriers
        // has to be run after the graph is optimized
        void schedule_graph();

        // cull passes from the graph that are not used
        void optimize();

        void destroyManagedResources();
    public:

        FrameGraph(render::IDeviceContext& context)
            : mContext(context)
        { }

        class AccessBuilder {
            FrameGraph& mFrameGraph;
            ResourceAccess& mAccess;
            Handle mHandle;

        public:
            AccessBuilder(FrameGraph& graph, ResourceAccess& access, Handle handle)
                : mFrameGraph(graph)
                , mAccess(access)
                , mHandle(handle)
            { }

            operator Handle() const { return mHandle; }

            AccessBuilder& override_srv(D3D12_SHADER_RESOURCE_VIEW_DESC desc) { return override_desc(desc); }
            AccessBuilder& override_uav(D3D12_UNORDERED_ACCESS_VIEW_DESC desc) { return override_desc(desc); }
            AccessBuilder& override_rtv(D3D12_RENDER_TARGET_VIEW_DESC desc) { return override_desc(desc); }
            AccessBuilder& override_dsv(D3D12_DEPTH_STENCIL_VIEW_DESC desc) { return override_desc(desc); }

            AccessBuilder& srv(D3D12_SHADER_RESOURCE_VIEW_DESC desc) { return override_desc(desc); }
            AccessBuilder& uav(D3D12_UNORDERED_ACCESS_VIEW_DESC desc) { return override_desc(desc); }
            AccessBuilder& rtv(D3D12_RENDER_TARGET_VIEW_DESC desc) { return override_desc(desc); }
            AccessBuilder& dsv(D3D12_DEPTH_STENCIL_VIEW_DESC desc) { return override_desc(desc); }

            AccessBuilder& override_desc(D3D12_SHADER_RESOURCE_VIEW_DESC desc);
            AccessBuilder& override_desc(D3D12_UNORDERED_ACCESS_VIEW_DESC desc);
            AccessBuilder& override_desc(D3D12_RENDER_TARGET_VIEW_DESC desc);
            AccessBuilder& override_desc(D3D12_DEPTH_STENCIL_VIEW_DESC desc);

            AccessBuilder& overrideView(D3D12_SHADER_RESOURCE_VIEW_DESC desc) { return override_srv(desc); }
            AccessBuilder& overrideView(D3D12_UNORDERED_ACCESS_VIEW_DESC desc) { return override_uav(desc); }
            AccessBuilder& overrideView(D3D12_RENDER_TARGET_VIEW_DESC desc) { return override_rtv(desc); }
            AccessBuilder& overrideView(D3D12_DEPTH_STENCIL_VIEW_DESC desc) { return override_dsv(desc); }

            AccessBuilder& withLayout(D3D12_BARRIER_LAYOUT value);
            AccessBuilder& withAccess(D3D12_BARRIER_ACCESS value);
            AccessBuilder& withSyncPoint(D3D12_BARRIER_SYNC value);
        };

#if 0
        class CreateAccessBuilder {
            FrameGraph& mFrameGraph;
            ResourceAccess& mCreateAccess;
            ResourceAccess& mWriteAccess;
            Handle mHandle;

        public:
            CreateAccessBuilder(FrameGraph& graph, ResourceAccess& create, ResourceAccess& write, Handle handle)
                : mFrameGraph(graph)
                , mCreateAccess(create)
                , mWriteAccess(write)
                , mHandle(handle)
            { }

            operator Handle() const { return mHandle; }

            CreateAccessBuilder& srv(D3D12_SHADER_RESOURCE_VIEW_DESC desc) {
                return overrideView(desc);
            }

            CreateAccessBuilder& uav(D3D12_UNORDERED_ACCESS_VIEW_DESC desc) {
                return overrideView(desc);
            }

            CreateAccessBuilder& rtv(D3D12_RENDER_TARGET_VIEW_DESC desc) {
                return overrideView(desc);
            }

            CreateAccessBuilder& dsv(D3D12_DEPTH_STENCIL_VIEW_DESC desc) {
                return overrideView(desc);
            }

            CreateAccessBuilder& overrideView(D3D12_SHADER_RESOURCE_VIEW_DESC desc);
            CreateAccessBuilder& overrideView(D3D12_UNORDERED_ACCESS_VIEW_DESC desc);
            CreateAccessBuilder& overrideView(D3D12_RENDER_TARGET_VIEW_DESC desc);
            CreateAccessBuilder& overrideView(D3D12_DEPTH_STENCIL_VIEW_DESC desc);

            CreateAccessBuilder& withLayout(D3D12_BARRIER_LAYOUT value);
            CreateAccessBuilder& withAccess(D3D12_BARRIER_ACCESS value);
            CreateAccessBuilder& withSyncPoint(D3D12_BARRIER_SYNC value);
        };
#endif

        class PassBuilder {
            FrameGraph& mFrameGraph;
            RenderPass& mRenderPass;

            ResourceAccess& addWriteAccess(Handle handle, sm::StringView name, Usage access);

        public:
            PassBuilder(FrameGraph& graph, RenderPass& pass)
                : mFrameGraph(graph)
                , mRenderPass(pass)
            { }

            AccessBuilder read(Handle handle,       sm::StringView name, Usage access);
            AccessBuilder write(Handle handle,      sm::StringView name, Usage access);
            AccessBuilder create(ResourceInfo info, sm::StringView name, Usage access);

            void side_effects(bool effects);

            PassBuilder& hasSideEffects(bool effects = true) {
                side_effects(effects);
                return *this;
            }

            template<typename F> requires std::invocable<F, RenderContext&>
            void bind(F&& execute) {
                mRenderPass.execute = execute;
            }
        };

        template<typename F> requires std::invocable<F, render::IDeviceContext&>
        auto& newDeviceData(F&& setup) {
            using ActualData = std::invoke_result_t<F, render::IDeviceContext&>;
            struct DeviceData final : IDeviceData {
                F exec;
                ActualData data;

                void setup(render::IDeviceContext& context) override {
                    data = std::move(exec(context));
                }

                bool has_type(uint32 index) const override {
                    return TypeIndex<ActualData>::index() == index;
                }

                DeviceData(F&& exec) : exec(std::move(exec)) { }
            };

            uint32 index = TypeIndex<ActualData>::index();
            if (auto it = mDeviceData.find(index); it != mDeviceData.end()) {
                auto& [_, result] = *it;
                return static_cast<DeviceData*>(result.get())->data;
            }

            auto [it, _] = mDeviceData.emplace(index, new DeviceData(std::forward<F>(setup)));
            auto& [_, data] = *it;

            data->setup(mContext);

            return static_cast<DeviceData*>(data.get())->data;
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

        render::IDeviceContext& getContext() { return mContext; }

        // destroy the render passes and resources
        void reset();

        void resize_frame_data(uint size);

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
