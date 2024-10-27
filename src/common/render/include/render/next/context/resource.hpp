#pragma once

#include "render/base/object.hpp"
#include "render/next/device.hpp"
#include "render/next/context/core.hpp"

#include <memory>

#include <D3D12MemAlloc.h>

namespace sm::render::next {
    class DeviceResource {
    protected:
        Object<D3D12MA::Allocation> mResource;

    public:
        DeviceResource(CoreContext& context, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc, D3D12MA::ALLOCATION_DESC alloc);

        ID3D12Resource *get() const noexcept { return mResource->GetResource(); }
        D3D12MA::Allocation *allocation() const noexcept { return mResource.get(); }
        D3D12_GPU_VIRTUAL_ADDRESS deviceAddress() const noexcept { return get()->GetGPUVirtualAddress(); }

        DeviceResource() noexcept = default;

        void reset() noexcept { mResource.reset(); }
    };

    class TextureResource : public DeviceResource {
    public:
        TextureResource() noexcept = default;
        TextureResource(CoreContext& context, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc, bool comitted);
    };

    class BufferResource : public DeviceResource {
    public:
        BufferResource() noexcept = default;
        BufferResource(CoreContext& context, D3D12_RESOURCE_STATES state, uint64 width, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heap);

        void write(const void *data, size_t size);
    };

    /// @brief a per frame resource buffer
    /// holds N resources, one for each frame
    template<typename T>
    class FrameResource {
        std::unique_ptr<T[]> mResources;

    protected:
        FrameResource(UINT frames, auto&& init) {
            mResources = std::make_unique<T[]>(frames);
            for (UINT i = 0; i < frames; ++i) {
                mResources[i] = init(i);
            }
        }

        T& get(UINT frame) noexcept {
            return mResources[frame];
        }
    };

    struct ConstBufferData {
        Object<D3D12MA::Allocation> resource;
        D3D12_GPU_DESCRIPTOR_HANDLE cbvHandle;
        void *data;
    };

    class ConstBufferResourceBase : public FrameResource<ConstBufferData> {
        using Super = FrameResource<ConstBufferData>;

    public:
        ConstBufferResourceBase(UINT frames, size_t size);

        void update(UINT frame, const void *data, size_t size);
    };

    template<typename T>
    concept ConstBufferType = (alignof(T) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT == 0)
                           && std::is_standard_layout_v<T>
                           && std::is_trivial_v<T>;


    template<ConstBufferType T>
    class ConstBufferResource : public ConstBufferResourceBase {
        using Super = ConstBufferResourceBase;

    public:
        ConstBufferResource(UINT frames)
            : ConstBufferResourceBase(frames, sizeof(T))
        { }

        void update(UINT frame, const T& value) {
            Super::update(frame, reinterpret_cast<void*>(&value), sizeof(T));
        }
    };
}
