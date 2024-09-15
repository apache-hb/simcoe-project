#pragma once

#include "core/span.hpp"
#include "render/object.hpp"

#include <D3D12MemAlloc.h>

namespace sm::render {
    struct IDeviceContext;

    class BufferResource {
        Object<D3D12MA::Allocation> mAllocation;
        D3D12_GPU_VIRTUAL_ADDRESS mDeviceAddress = 0;

    public:
        BufferResource() = default;

        BufferResource(
            IDeviceContext& context,
            const D3D12_RESOURCE_DESC& desc,
            const D3D12MA::ALLOCATION_DESC& alloc,
            D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON
        ) noexcept;

        void reset() noexcept;
        void rename(std::string_view name) noexcept;

        D3D12_GPU_VIRTUAL_ADDRESS getDeviceAddress() const noexcept { return mDeviceAddress; }
        ID3D12Resource *getResource() const noexcept { return mAllocation->GetResource(); }
        D3D12MA::Allocation *getAllocation() const noexcept { return mAllocation.get(); }

        void *mapWriteOnly() noexcept;
        void unmap(D3D12_RANGE written) noexcept;
        void unmap() noexcept;

        template<StandardLayout T>
        T *mapWriteOnly() noexcept {
            return static_cast<T*>(mapWriteOnly());
        }
    };

    struct Resource {
        Object<ID3D12Resource> mResource;
        Object<D3D12MA::Allocation> mAllocation;
        D3D12_GPU_VIRTUAL_ADDRESS mGpuAddress = 0;

        Result map(const D3D12_RANGE *range, void **data);
        void unmap(const D3D12_RANGE *range);
        Result write(const void *data, size_t size);

        D3D12_GPU_VIRTUAL_ADDRESS getDeviceAddress() const noexcept { return mGpuAddress; }

        void reset();
        void rename(sm::StringView name);

        ID3D12Resource *get() const noexcept { return mResource.get(); }
    };

    template<typename T>
    struct ConstBuffer : Resource {
        void *mapped;

        void update(const T& value) {
            memcpy(mapped, &value, sizeof(T));
        }

        void init() {
            D3D12_RANGE read{0, 0};
            SM_ASSERT_HR(Resource::map(&read, &mapped));
        }
    };

    template<typename T>
    struct VertexUploadBuffer : Resource {
        D3D12_VERTEX_BUFFER_VIEW view;
        void *mapped;

        void update(sm::Span<const T> data) {
            memcpy(mapped, data.data(), data.size_bytes());
        }

        void init(size_t count) {
            D3D12_RANGE read{0, 0};
            SM_ASSERT_HR(Resource::map(&read, &mapped));

            view.BufferLocation = getDeviceAddress();
            view.SizeInBytes = sizeof(T) * count;
            view.StrideInBytes = sizeof(T);
        }

        D3D12_VERTEX_BUFFER_VIEW getView() const {
            return view;
        }
    };
}
