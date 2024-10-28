#pragma once

#include <map>
#include <memory>
#include <utility>
#include <typeindex>
#include <string>
#include <variant>
#include <vector>
#include <functional>

#include <directx/d3d12.h>

#include "render/next/components.hpp"
#include "render/next/context/resource.hpp"

namespace sm::graph {
    class RenderGraphBuilder;

    class Handle {
    public:
        size_t mIndex;

        Handle(size_t index) : mIndex(index) { }
    };

    class HandleData {
        std::string mName;
        ID3D12Resource *mResource;
        D3D12_RESOURCE_STATES mState;

    public:
        HandleData(std::string name, ID3D12Resource *resource, D3D12_RESOURCE_STATES state);

        std::string_view getName() const { return mName; }
        ID3D12Resource *get() const { return mResource; }
        D3D12_RESOURCE_STATES getState() const { return mState; }
    };

    class HandleUse {
    public:
        size_t mIndex;

        HandleUse(size_t index) : mIndex(index) { }
    };

    class HandleUseData {
        std::string mName;
        Handle mHandle;
        D3D12_RESOURCE_STATES mState;

    public:
        size_t rtvIndex = SIZE_MAX;
        size_t dsvIndex = SIZE_MAX;
        size_t srvIndex = SIZE_MAX;

        HandleUseData(std::string name, Handle handle, D3D12_RESOURCE_STATES state);

        Handle getHandle() const { return mHandle; }
        D3D12_RESOURCE_STATES getState() const { return mState; }
    };

    using ExecuteFn = std::function<void(ID3D12GraphicsCommandList*)>;

    struct RenderPassInfo {
        std::string name;
        D3D12_COMMAND_LIST_TYPE type;
        ExecuteFn execute;
    };

    class RenderPass {
    public:
        RenderPassInfo mInfo;
        RenderPass(RenderPassInfo info);
    };

    struct IDeviceData {
        virtual ~IDeviceData() = default;

        virtual void setup() = 0;
    };

    class HandleUseBuilder {
        RenderGraphBuilder& mGraph;
        HandleUse mHandleUse;

    public:
        HandleUseBuilder(RenderGraphBuilder& graph, HandleUse handle)
            : mGraph(graph)
            , mHandleUse(handle)
        { }

        operator Handle() const;

        D3D12_CPU_DESCRIPTOR_HANDLE createRenderTargetView();
        D3D12_CPU_DESCRIPTOR_HANDLE createRenderTargetView(D3D12_RENDER_TARGET_VIEW_DESC desc);

        D3D12_CPU_DESCRIPTOR_HANDLE createDepthStencilView();
        D3D12_CPU_DESCRIPTOR_HANDLE createDepthStencilView(D3D12_DEPTH_STENCIL_VIEW_DESC desc);

        D3D12_GPU_DESCRIPTOR_HANDLE createShaderResourceView();
        D3D12_GPU_DESCRIPTOR_HANDLE createShaderResourceView(D3D12_SHADER_RESOURCE_VIEW_DESC desc);
    };

    class RenderPassBuilder {
        RenderGraphBuilder& mGraph;

        std::string mName;
        D3D12_COMMAND_LIST_TYPE mType;

        void addToGraph(ExecuteFn fn);
    public:
        RenderPassBuilder(RenderGraphBuilder& graph, std::string name, D3D12_COMMAND_LIST_TYPE type)
            : mGraph(graph)
            , mName(std::move(name))
            , mType(type)
        { }

        HandleUseBuilder create(const std::string& name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc);
        HandleUseBuilder read(Handle handle, std::string name, D3D12_RESOURCE_STATES state);
        HandleUseBuilder write(Handle handle, std::string name, D3D12_RESOURCE_STATES state);

        template<typename F> requires (std::invocable<F, ID3D12GraphicsCommandList*>)
        void bind(F&& func) {
            addToGraph(std::forward<F>(func));
        }
    };

    struct DescriptorIndex {
        size_t index;
        D3D12_CPU_DESCRIPTOR_HANDLE host;
        D3D12_GPU_DESCRIPTOR_HANDLE device;
    };

    class RenderGraphBuilder {
        render::next::CoreContext& mContext;

        std::map<std::type_index, std::unique_ptr<IDeviceData>> mDeviceData;
        std::vector<HandleData> mHandles;
        std::vector<HandleUseData> mHandleUses;
        std::vector<RenderPass> mRenderPasses;
        std::vector<render::next::DeviceResource> mDeviceResources;

        template<typename T, typename... A> requires (std::derived_from<T, IDeviceData>)
        T *addDeviceData(A&&... args) {
            std::type_index index = std::type_index(typeid(T));
            if (auto it = mDeviceData.find(index); it != mDeviceData.end()) {
                auto& [_, result] = *it;
                return static_cast<T*>(result.get());
            }

            auto [it, _] = mDeviceData.emplace(index, new T{std::forward<A>(args)...});
            auto& [_, data] = *it;

            data->setup();

            return static_cast<T*>(data.get());
        }

    public:
        RenderGraphBuilder(render::next::CoreContext& context)
            : mContext(context)
        { }

        void addRenderPass(RenderPassInfo info);
        Handle newResourceHandle(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc);
        HandleUse newResourceUsage(std::string name, Handle handle, D3D12_RESOURCE_STATES state);

        Handle include(std::string name, ID3D12Resource *resource, D3D12_RESOURCE_STATES state);

        RenderPassBuilder newRenderPass(std::string name, D3D12_COMMAND_LIST_TYPE type);
        RenderPassBuilder graphics(std::string name);
        RenderPassBuilder compute(std::string name);

        template<typename T, typename... A> requires (std::constructible_from<T, A...>)
        T& dataClass(A&&... args) {
            class DeviceData final : public IDeviceData {
                std::tuple<A...> mArgs;
                T mData;

            public:
                void setup() override {
                    mData = T{std::forward<A>(mArgs)...};
                }

                T& data() { return mData; }

                DeviceData(A&&... args)
                    : mArgs(std::forward<A>(args)...)
                { }
            };

            return addDeviceData<DeviceData>(std::forward<A>(args)...)->data();
        }

        template<typename F>
        std::invoke_result_t<F>& data(F&& func) {
            using DataType = decltype(func());
            class DeviceData final : public IDeviceData {
                F mGetter;
                DataType mData;

            public:
                DeviceData(F&& getter)
                    : mGetter(std::move(getter))
                { }

                void setup() override {
                    mData = std::move(mGetter());
                }

                DataType& data() { return mData; }
            };

            return addDeviceData<DeviceData>(std::forward<F>(func))->data();
        }

        Handle getHandle(HandleUse handle) const;
        DescriptorIndex createRenderTargetView(Handle handle, const D3D12_RENDER_TARGET_VIEW_DESC *desc);
        DescriptorIndex createDepthStencilView(Handle handle, const D3D12_DEPTH_STENCIL_VIEW_DESC *desc);
        DescriptorIndex createShaderResourceView(Handle handle, const D3D12_SHADER_RESOURCE_VIEW_DESC *desc);

        HandleData& getHandleData(Handle handle) { return mHandles[handle.mIndex]; }
        HandleUseData& getHandleUseData(HandleUse handle) { return mHandleUses[handle.mIndex]; }
    };
}
