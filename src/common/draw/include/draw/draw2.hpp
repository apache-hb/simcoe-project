#pragma once

#include "core/adt/array.hpp"

#include "draw/forward_plus.hpp"

#include "render/resource.hpp"

namespace sm::draw {
    constexpr inline D3D12MA::ALLOCATION_DESC kMemoryInfo = {
        .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
        .HeapType = D3D12_HEAP_TYPE_UPLOAD,
    };

    template<StandardLayout T>
    struct DeviceBuffer {
        render::BufferResource resource;
        T *mapped;

        sm::UniqueArray<T> buffer;

        DeviceBuffer(render::IDeviceContext& context, size_t count) noexcept
            : resource(context, CD3DX12_RESOURCE_DESC::Buffer(sizeof(T) * count), kMemoryInfo)
            , mapped(resource.mapWriteOnly<T>())
            , buffer(count)
        { }
    };

    struct WorldData {
        ViewportData viewports[kMaxViewports];
        uint activeViewport;
    };
}
