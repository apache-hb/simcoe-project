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
        using Super = DeviceResource;
    public:
        TextureResource() noexcept = default;
        TextureResource(CoreContext& context, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_DESC desc, bool comitted);
    };

    class AnyBufferResource : public DeviceResource {
        using Super = DeviceResource;
    public:
        AnyBufferResource() noexcept = default;
        AnyBufferResource(CoreContext& context, uint64_t width, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heap);

        void write(const void *data, size_t size);

        void *map(size_t size);
        void unmap(void *mapped, const D3D12_RANGE *written) noexcept;
    };

    template<typename T>
    class BufferResource : public AnyBufferResource {
        using Super = AnyBufferResource;

    public:
        BufferResource() noexcept = default;
        BufferResource(CoreContext& context, uint64_t count, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heap)
            : Super(context, count * sizeof(T), state, flags, heap)
        { }

        void write(std::span<const T> data) {
            Super::write((void*)data.data(), data.size_bytes());
        }

        T *map(size_t size) {
            return reinterpret_cast<T*>(Super::map(size * sizeof(T)));
        }

        void unmap(T *mapped, const D3D12_RANGE *written) noexcept {
            Super::unmap((void*)mapped, written);
        }
    };

    class AnyMultiBufferResource : public AnyBufferResource {
        using Super = AnyBufferResource;

        uint16_t mElementCount;
        void *mMapped;

        void *getElementData(size_t index, size_t stride) const;

    protected:
        D3D12_GPU_VIRTUAL_ADDRESS getElementDeviceAddress(size_t index, size_t stride) const;

    public:
        AnyMultiBufferResource() noexcept = default;
        AnyMultiBufferResource(CoreContext& context, size_t count, size_t size, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heap);

        void update(UINT frame, const void *data, size_t stride);
    };

    template<typename T>
    concept ConstBufferType = std::is_standard_layout_v<T>
                           && std::is_trivial_v<T>;

    template<ConstBufferType T>
    class MultiBufferResource : public AnyMultiBufferResource {
        using Super = AnyMultiBufferResource;

    public:
        MultiBufferResource() noexcept = default;
        MultiBufferResource(CoreContext& context, size_t count, D3D12_RESOURCE_STATES state, D3D12_RESOURCE_FLAGS flags, D3D12_HEAP_TYPE heap)
            : Super(context, count, sizeof(T), state, flags, heap)
        { }

        void updateElement(UINT frame, const T& value) {
            update(frame, reinterpret_cast<const void*>(&value), sizeof(T));
        }

        D3D12_GPU_VIRTUAL_ADDRESS elementAddress(UINT frame) const {
            return getElementDeviceAddress(frame, sizeof(T));
        }
    };
}
