#pragma once

#include "core/span.hpp"
#include "render/object.hpp"

#include <D3D12MemAlloc.h>

namespace sm::render {
    struct Resource {
        Object<ID3D12Resource> mResource;
        Object<D3D12MA::Allocation> mAllocation;

        Result map(const D3D12_RANGE *range, void **data);
        void unmap(const D3D12_RANGE *range);
        Result write(const void *data, size_t size);

        D3D12_GPU_VIRTUAL_ADDRESS getDeviceAddress() { return get_gpu_address(); }

        D3D12_GPU_VIRTUAL_ADDRESS get_gpu_address();
        void reset();
        void rename(sm::StringView name);

        ID3D12Resource *get() const { return mResource.get(); }
    };

    template<typename T>
    struct ConstBuffer : Resource {
        T data;
        void *mapped;

        void update() {
            memcpy(mapped, &data, sizeof(T));
        }

        void init() {
            D3D12_RANGE read{0, 0};
            SM_ASSERT_HR(Resource::map(&read, &mapped));
        }
    };

    template<typename T>
    struct VertexBuffer : Resource {
        D3D12_VERTEX_BUFFER_VIEW view;
        void *mapped;

        void update(sm::Span<const T> data) {
            memcpy(mapped, data.data(), data.size_bytes());
        }

        void init(size_t count) {
            D3D12_RANGE read{0, 0};
            SM_ASSERT_HR(Resource::map(&read, &mapped));

            view.BufferLocation = get_gpu_address();
            view.SizeInBytes = sizeof(T) * count;
            view.StrideInBytes = sizeof(T);
        }

        D3D12_VERTEX_BUFFER_VIEW get_view() const {
            return view;
        }
    };
}
