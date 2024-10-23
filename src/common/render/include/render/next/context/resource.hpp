#pragma once

#include "render/base/object.hpp"

#include <memory>

#include <D3D12MemAlloc.h>

namespace sm::render::next {
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
