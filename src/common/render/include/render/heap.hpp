#pragma once

#include "core/bitmap.hpp"
#include "render/object.hpp"

namespace sm::render {
    template<D3D12_DESCRIPTOR_HEAP_TYPE D>
    class DescriptorPool {
        Object<ID3D12DescriptorHeap> mHeap;
        uint mDescriptorSize;
        uint mCapacity;
        sm::BitMap mAllocator;

    public:
        constexpr static D3D12_DESCRIPTOR_HEAP_TYPE kHeapType = D;

        enum class Index : uint { eInvalid = UINT_MAX };

        bool test_bit(Index index) const { return mAllocator.test(sm::BitMap::Index{size_t(index)}); }

        Result init(ID3D12Device1 *device, uint capacity, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
            const D3D12_DESCRIPTOR_HEAP_DESC kDesc = {
                .Type = kHeapType,
                .NumDescriptors = capacity,
                .Flags = flags,
                .NodeMask = 0,
            };

            mDescriptorSize = device->GetDescriptorHandleIncrementSize(kHeapType);
            mCapacity = capacity;
            mAllocator.resize(capacity);

            return device->CreateDescriptorHeap(&kDesc, IID_PPV_ARGS(&mHeap));
        }

        Index allocate() {
            auto index = mAllocator.allocate();
            CTASSERTF(index < mCapacity, "DescriptorPool: allocate out of range. index %u capacity %u", enum_cast<uint>(index), mCapacity);
            CTASSERTF(index != sm::BitMap::eInvalid, "DescriptorPool: allocate out of descriptors. size %u", mCapacity);
            return Index{uint(index)};
        }

        void release(Index index) {
            mAllocator.release(sm::BitMap::Index{size_t(index)});
        }

        void safe_release(Index index) {
            if (index != Index::eInvalid) release(index);
        }

        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle(Index index) {
            CTASSERTF(test_bit(index), "DescriptorPool: cpu_handle index %u is not set", enum_cast<uint>(index));
            D3D12_CPU_DESCRIPTOR_HANDLE handle = mHeap->GetCPUDescriptorHandleForHeapStart();
            handle.ptr += enum_cast<uint>(index) * mDescriptorSize;
            return handle;
        }

        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle(Index index) {
            CTASSERTF(test_bit(index), "DescriptorPool: gpu_handle index %u is not set", enum_cast<uint>(index));
            D3D12_GPU_DESCRIPTOR_HANDLE handle = mHeap->GetGPUDescriptorHandleForHeapStart();
            handle.ptr += enum_cast<uint>(index) * mDescriptorSize;
            return handle;
        }

        void reset() { mHeap.reset(); mAllocator.reset(); }
        ID3D12DescriptorHeap *get() const { return mHeap.get(); }

        uint get_capacity() const { return mCapacity; }
        uint get_used() const {
            // freecount returns the number of free bits in the bitset,
            // the bitset can be larger than the number of descriptors
            uint freecount = mAllocator.freecount();
            auto diff = mCapacity - freecount;
            return diff < mCapacity ? diff : mCapacity;
        }
    };

    using RtvPool = DescriptorPool<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>;
    using DsvPool = DescriptorPool<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>;
    using SrvPool = DescriptorPool<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>;

    using RtvIndex = RtvPool::Index;
    using DsvIndex = DsvPool::Index;
    using SrvIndex = SrvPool::Index;
}
