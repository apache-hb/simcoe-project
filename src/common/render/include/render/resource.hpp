#pragma once

#include "render/object.hpp"

#include <D3D12MemAlloc.h>

namespace sm::render {
    struct Resource {
        Object<ID3D12Resource> mResource;
        Object<D3D12MA::Allocation> mAllocation;

        Result map(const D3D12_RANGE *range, void **data);
        void unmap(const D3D12_RANGE *range);

        D3D12_GPU_VIRTUAL_ADDRESS get_gpu_address();
        void reset();
        void rename(sm::StringView name);

        ID3D12Resource *get() const { return mResource.get(); }
    };
}
