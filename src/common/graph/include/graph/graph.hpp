#pragma once

#include <map>
#include <memory>
#include <utility>
#include <typeindex>
#include <string>
#include <vector>
#include <functional>

#include <directx/d3d12.h>

namespace sm::graph {
    class RenderGraph;

    class Handle {
    public:
        size_t mIndex;

        Handle(size_t index);
    };

    class HandleData {
    public:
        std::string mName;
        D3D12_RESOURCE_STATES mState;
        D3D12_RESOURCE_DESC mDesc;

        HandleData(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc);
    };

    struct HandleCreateInfo {
        Handle handle;
    };

    struct HandleReadInfo {
        Handle handle;
        std::string name;
        D3D12_RESOURCE_STATES state;
    };

    struct HandleWriteInfo {
        Handle handle;
        std::string name;
        D3D12_RESOURCE_STATES state;
    };

    using ExecuteFn = std::function<void(ID3D12GraphicsCommandList*)>;

    struct RenderPassInfo {
        std::string name;
        D3D12_COMMAND_LIST_TYPE type;
        std::vector<HandleCreateInfo> create;
        std::vector<HandleReadInfo> read;
        std::vector<HandleWriteInfo> write;
        ExecuteFn execute;
    };

    class RenderPass {
    public:
        RenderPass(RenderPassInfo info);
    };

    struct IDeviceData {
        virtual ~IDeviceData() = default;

        virtual void setup() = 0;
    };

    class RenderPassBuilder {
        RenderGraph& mGraph;

        std::string mName;
        D3D12_COMMAND_LIST_TYPE mType;
        std::vector<HandleCreateInfo> mCreate;
        std::vector<HandleReadInfo> mRead;
        std::vector<HandleWriteInfo> mWrite;

        void addToGraph(ExecuteFn fn);
    public:
        RenderPassBuilder(RenderGraph& graph, std::string name, D3D12_COMMAND_LIST_TYPE type)
            : mGraph(graph)
            , mName(std::move(name))
            , mType(type)
        { }

        Handle create(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc);
        void read(Handle handle, std::string name, D3D12_RESOURCE_STATES state);
        void write(Handle handle, std::string name, D3D12_RESOURCE_STATES state);

        template<typename F> requires (std::invocable<F, ID3D12GraphicsCommandList*>)
        void bind(F&& func) {
            addToGraph(std::forward<F>(func));
        }
    };

    class RenderGraph {
        std::map<std::type_index, std::unique_ptr<IDeviceData>> mDeviceData;
        std::vector<HandleData> mHandles;
        std::vector<RenderPass> mRenderPasses;

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
        void addRenderPass(RenderPassInfo info);
        Handle newResourceHandle(std::string name, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc);

        Handle include(ID3D12Resource *resource, D3D12_RESOURCE_STATES state);

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

        D3D12_CPU_DESCRIPTOR_HANDLE rtv(Handle handle);
        D3D12_CPU_DESCRIPTOR_HANDLE dsv(Handle handle);
        D3D12_GPU_DESCRIPTOR_HANDLE srv(Handle handle);
    };
}
