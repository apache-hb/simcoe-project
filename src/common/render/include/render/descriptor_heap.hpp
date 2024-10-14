#pragma once

#include "core/allocators/bitmap.hpp"

#include "render/base/object.hpp"

namespace sm::render::next {
    class DescriptorPoolBase {
        static constexpr inline size_t kInitialAllocatorSize = 64;
        sm::BitMapIndexAllocator mAllocator{kInitialAllocatorSize};

        Object<ID3D12DescriptorHeap> mHeap;
        UINT mDescriptorSize;
        bool mIsShaderVisible;

        D3D12_CPU_DESCRIPTOR_HANDLE mHostHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE mDeviceHandle;

    protected:
        DescriptorPoolBase(ID3D12DescriptorHeap *heap, UINT descriptorSize, bool isShaderVisible) noexcept
            : mHeap(heap)
            , mDescriptorSize(descriptorSize)
            , mIsShaderVisible(isShaderVisible)
        { }

        DescriptorPoolBase(ID3D12Device1 *device, D3D12_DESCRIPTOR_HEAP_DESC desc);

    public:
        UINT getDescriptorSize() const noexcept { return mDescriptorSize; }
        bool isShaderVisible() const noexcept { return mIsShaderVisible; }

        ~DescriptorPoolBase() noexcept;
    };

    template<D3D12_DESCRIPTOR_HEAP_TYPE H>
    class DescriptorPool : public DescriptorPoolBase {
        using Super = DescriptorPoolBase;
    public:
    };
}

namespace sm::render {
    using HostDescriptorHandle = D3D12_CPU_DESCRIPTOR_HANDLE;
    using DeviceDescriptorHandle = D3D12_GPU_DESCRIPTOR_HANDLE;

    class DescriptorPoolBase {
        static constexpr inline size_t kInitialAllocatorSize = 64;
        sm::BitMapIndexAllocator mAllocator{kInitialAllocatorSize};

        Object<ID3D12DescriptorHeap> mHeap;
        uint mDescriptorSize = 0;
        bool mIsShaderVisible = false;

        HostDescriptorHandle mFirstHostHandle = {};
        DeviceDescriptorHandle mFirstDeviceHandle = {};

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

        HostDescriptorHandle getHostDescriptorHandle(size_t index) const noexcept;
        DeviceDescriptorHandle getDeviceDescriptorHandle(size_t index) const noexcept;

        HostDescriptorHandle getBaseHostHandle() const noexcept { return mFirstHostHandle; }
        DeviceDescriptorHandle getBaseDeviceHandle() const noexcept { return mFirstDeviceHandle; }

        void reset() noexcept {
            mHeap.reset();
            mAllocator.clear();
        }

        uint getCapacity() const noexcept { return mAllocator.capacity(); }
        uint countUsedHandles() const noexcept { return mAllocator.popcount(); }
    };

    template<D3D12_DESCRIPTOR_HEAP_TYPE H>
    class DescriptorPool : public DescriptorPoolBase {
        using Super = DescriptorPoolBase;
    public:
        enum class Index : uint { eInvalid = DescriptorPoolBase::kInvalidIndex };
        using Super::Super;

        Result init(ID3D12Device1 *device, uint capacity, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
            const D3D12_DESCRIPTOR_HEAP_DESC kDesc = {
                .Type = H,
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

        HostDescriptorHandle cpu_handle(Index index) const noexcept {
            CTASSERT(Super::test(uint(index)));
            return Super::getHostDescriptorHandle(uint(index));
        }

        DeviceDescriptorHandle gpu_handle(Index index) const noexcept {
            CTASSERT(Super::test(uint(index)));
            return Super::getDeviceDescriptorHandle(uint(index));
        }
    };

    using RtvPool = DescriptorPool<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>;
    using DsvPool = DescriptorPool<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>;
    using SrvPool = DescriptorPool<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>;

    using RtvIndex = RtvPool::Index;
    using DsvIndex = DsvPool::Index;
    using SrvIndex = SrvPool::Index;
}
