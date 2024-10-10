#pragma once

#include "core/adt/array.hpp"
#include "core/string.hpp"
#include "core/adt/vector.hpp"
#include "core/typeindex.hpp"
#include "core/map.hpp"

#include "math/math.hpp"

#include "render/base/resource.hpp"
#include "render/base/format.hpp"

#include "render/descriptor_heap.hpp"

#include "render/graph/handle.hpp"
#include "render/graph/events.hpp"
#include "render/graph/render_pass.hpp"

#include "render.reflect.h"

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
        ClearType getClearType() const { return mClearType; }
        DXGI_FORMAT getFormat() const { return mFormat; }

        float getClearDepth() const;
        math::float4 getClearColour() const;

        static Clear empty();
        static Clear colour(math::float4 value, DXGI_FORMAT format);
        static Clear depthStencil(float depth, uint8 stencil, DXGI_FORMAT format);
    };

    struct Tex2dInfo {
        math::uint2 size;
    };

    struct ArrayInfo {
        uint stride;
        uint length;

        uint getSize() const { return stride * length; }
    };

    using ResourceSize = std::variant<
        Tex2dInfo,
        ArrayInfo
    >;

    class ResourceInfo {
        ResourceSize mResourceSize;
        DXGI_FORMAT mFormat;
        Clear mClearValue;
        bool mBuffered = false;

    public:
        ResourceInfo(ResourceSize size, DXGI_FORMAT format, Clear clear = Clear::empty(), bool buffered = false)
            : mResourceSize(size)
            , mFormat(format)
            , mClearValue(clear)
            , mBuffered(buffered)
        { }

        static ResourceInfo tex2d(math::uint2 size, DXGI_FORMAT format) {
            return ResourceInfo(Tex2dInfo { size }, format);
        }

        template<typename T> requires (std::is_standard_layout_v<T> && std::is_trivial_v<T>)
        static ResourceInfo structuredBuffer(uint size) {
            return ResourceInfo(ArrayInfo { sizeof(T), size }, DXGI_FORMAT_UNKNOWN);
        }

        template<typename T> requires (render::bufferFormatOf<T>() != DXGI_FORMAT_UNKNOWN)
        static ResourceInfo arrayOf(uint length) {
            return ResourceInfo(ArrayInfo { sizeof(T), length }, render::bufferFormatOf<T>());
        }

        ResourceInfo& clear(Clear clear) { mClearValue = clear; return *this; }

        ResourceInfo& clearColour(math::float4 colour, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN) {
            mClearValue = Clear::colour(colour, (format == DXGI_FORMAT_UNKNOWN) ? getFormat() : format);
            return *this;
        }

        ResourceInfo& clearDepthStencil(float depth, uint8 stencil, DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN) {
            mClearValue = Clear::depthStencil(depth, stencil, (format == DXGI_FORMAT_UNKNOWN) ? getFormat() : format);
            return *this;
        }

        ResourceInfo& buffered(bool value = true) { mBuffered = value; return *this; }

        ResourceSize getSize() const { return mResourceSize; }
        DXGI_FORMAT getFormat() const { return mFormat; }
        Clear getClearValue() const { return mClearValue; }
        bool isBuffered() const { return mBuffered; }

        const Tex2dInfo *asTex2d() const { return std::get_if<Tex2dInfo>(&mResourceSize); }
        const ArrayInfo *asArray() const { return std::get_if<ArrayInfo>(&mResourceSize); }
        bool isTex2d() const { return asTex2d() != nullptr; }
        bool isArray() const { return asArray() != nullptr; }
    };

    struct DescriptorPack {
        // will be filled depending on the type of resource
        render::RtvIndex rtv = render::RtvIndex::eInvalid;
        render::DsvIndex dsv = render::DsvIndex::eInvalid;
        render::SrvIndex srv = render::SrvIndex::eInvalid;
        render::SrvIndex uav = render::SrvIndex::eInvalid;

        void release(render::IDeviceContext& context);
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

        struct HandleRange {
            uint front;
            uint back;

            uint length() const { return back - front; }

            template<typename T>
            sm::Span<T> viewOfData(T *data) const {
                return {data + front, length()};
            }
        };

        struct ResourceHandle {
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

            // if this is an external resource this is the handle to the resource
            ID3D12Resource *external = nullptr;
            DescriptorPack descriptors;

            // the range of resources that this handle names
            // usually start == end, but if the resource is buffered
            // this should be a range of resources the same length
            // as the swapchain backbuffer length
            HandleRange resources{UINT_MAX, UINT_MAX};

            // override the default generation of the resource descriptions
            std::optional<D3D12_SHADER_RESOURCE_VIEW_DESC> srv_desc;
            std::optional<D3D12_UNORDERED_ACCESS_VIEW_DESC> uav_desc;
            std::optional<D3D12_RENDER_TARGET_VIEW_DESC> rtv_desc;
            std::optional<D3D12_DEPTH_STENCIL_VIEW_DESC> dsv_desc;

            bool isImported() const { return type == ResourceType::eImported; }
            bool isManaged() const { return type == ResourceType::eManaged || type == ResourceType::eTransient; }
            bool isUsed() const { return isManaged() || refcount > 0; }

            bool isBuffered() const { return info.isBuffered(); }
            bool isExternal() const { return type == ResourceType::eImported; }
            bool isInternal() const { return type == ResourceType::eManaged || type == ResourceType::eTransient; }
        };

        render::IDeviceContext& mContext;

        sm::Vector<RenderPass> mRenderPasses;
        sm::Vector<ResourceHandle> mHandles;

        struct ResourceData {
            render::Resource resource;
            DescriptorPack descriptors;

            ID3D12Resource *getResource() { return resource.get(); }
            DescriptorPack& getDescriptors() { return descriptors; }
        };

        sm::Vector<ResourceData> mResources;

        ResourceData& getResourceData(Handle handle);
        sm::Span<ResourceData> getAllResourceData(Handle handle);
        DescriptorPack& getResourceDescriptors(Handle handle);

        sm::HashMap<uint32, sm::UniquePtr<IDeviceData>> mDeviceData;

        uint mFrameIndex;
        int mFrameGraphVersion = 0;

        FrameSchedule mFrameSchedule;


        ///
        /// frame data resources
        ///
        sm::Vector<FrameCommandData> mFrameData;

        void reset_frame_commands();
        void init_frame_commands();
        CommandListHandle addCommandList(render::CommandListType type);
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

        bool isImported(Handle handle) const;

        RenderPassHandle findNextHandleUse(size_t start, Handle handle) const;

        Usage getNextAccess(size_t start, Handle handle) const;

        bool forEachAccess(size_t start, Handle handle, auto&& fn) const {
            if (start >= mRenderPasses.size()) return true;

            for (size_t i = start; i < mRenderPasses.size(); i++) {
                auto& pass = mRenderPasses[i];
                if (!pass.isUsed()) continue;

                if (pass.uses_handle(handle)) {
                    if (!fn(pass.getHandleAccess(handle))) {
                        return false;
                    }
                }
            }

            return true;
        }

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

            AccessBuilder& override_desc(D3D12_SHADER_RESOURCE_VIEW_DESC desc);
            AccessBuilder& override_desc(D3D12_UNORDERED_ACCESS_VIEW_DESC desc);
            AccessBuilder& override_desc(D3D12_RENDER_TARGET_VIEW_DESC desc);
            AccessBuilder& override_desc(D3D12_DEPTH_STENCIL_VIEW_DESC desc);

            AccessBuilder& withStates(D3D12_RESOURCE_STATES value);
            AccessBuilder& withLayout(D3D12_BARRIER_LAYOUT value);
            AccessBuilder& withAccess(D3D12_BARRIER_ACCESS value);
            AccessBuilder& withSyncPoint(D3D12_BARRIER_SYNC value);
        };

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

            AccessBuilder read(Handle handle, sm::StringView name);

            PassBuilder& hasSideEffects(bool effects = true);

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

        void setFrameIndex(uint index) { mFrameIndex = index; }

        // also destroy the device data
        void reset_device_data();
        void compile();
        void execute();
    };

    using PassBuilder = FrameGraph::PassBuilder;
}
