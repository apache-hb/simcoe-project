#pragma once

#include "core/allocators/bitmap_allocator.hpp"

#include "core/units.hpp"
#include "render/object.hpp"

namespace sm::render {
    class DescriptorPoolBase {
        static constexpr inline size_t kInitialAllocatorSize = 64;
        sm::BitMapIndexAllocator mAllocator{kInitialAllocatorSize};

        Object<ID3D12DescriptorHeap> mHeap;
        uint mDescriptorSize = 0;
        bool mIsShaderVisible = false;

        D3D12_CPU_DESCRIPTOR_HANDLE mFirstHostHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE mFirstDeviceHandle = {};

    protected:
        HRESULT createDescriptorHeap(ID3D12Device1 *device, const D3D12_DESCRIPTOR_HEAP_DESC &desc) noexcept;

        size_t allocate() noexcept { return mAllocator.allocateIndex(); }
        bool test(size_t index) const noexcept { return mAllocator.test(index); }

        void release(size_t index) noexcept;
        bool tryRelease(size_t index) noexcept;

    public:
        static constexpr inline uint kInvalidIndex = UINT_MAX;

        bool isShaderVisible() const noexcept { return mIsShaderVisible; }
        uint getDescriptorSize() const noexcept { return mDescriptorSize; }
        ID3D12DescriptorHeap *get() const noexcept { return mHeap.get(); }

        D3D12_GPU_DESCRIPTOR_HANDLE getBaseDeviceHandle() const noexcept {
            CTASSERT(isShaderVisible());
            return mFirstDeviceHandle;
        }

        D3D12_CPU_DESCRIPTOR_HANDLE getBaseHostHandle() const noexcept { return mFirstHostHandle; }

        void reset() noexcept {
            mHeap.reset();
            mAllocator.clear();
        }

        uint getCapacity() const noexcept { return mAllocator.capacity(); }
        uint countUsedHandles() const noexcept { return mAllocator.popcount(); }
    };

    template<D3D12_DESCRIPTOR_HEAP_TYPE THeapType>
    class DescriptorPool : public DescriptorPoolBase {
        using Super = DescriptorPoolBase;
    public:
        enum class Index : uint { eInvalid = DescriptorPoolBase::kInvalidIndex };
        using Super::Super;

        Result init(ID3D12Device1 *device, uint capacity, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
            const D3D12_DESCRIPTOR_HEAP_DESC kDesc = {
                .Type = THeapType,
                .NumDescriptors = capacity,
                .Flags = flags,
                .NodeMask = 0,
            };

            return Super::createDescriptorHeap(device, kDesc);
        }

        Index allocate() {
            size_t index = Super::allocate();
            CTASSERT(index != Super::kInvalidIndex);
            return Index(index);
        }

        void release(Index index) {
            Super::release(uint(index));
        }

        bool safe_release(Index index) {
            return Super::tryRelease(uint(index));
        }

        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle(Index index) const noexcept {
            CTASSERT(Super::test(uint(index)));
            D3D12_CPU_DESCRIPTOR_HANDLE handle = Super::getBaseHostHandle();
            handle.ptr += enum_cast<uint>(index) * Super::getDescriptorSize();
            return handle;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle(Index index) const noexcept {
            CTASSERT(Super::test(uint(index)));
            D3D12_GPU_DESCRIPTOR_HANDLE handle = Super::getBaseDeviceHandle();
            handle.ptr += enum_cast<uint>(index) * Super::getDescriptorSize();
            return handle;
        }
    };

    using RtvPool = DescriptorPool<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>;
    using DsvPool = DescriptorPool<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>;
    using SrvPool = DescriptorPool<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>;

    using RtvIndex = RtvPool::Index;
    using DsvIndex = DsvPool::Index;
    using SrvIndex = SrvPool::Index;
}
