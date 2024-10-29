#pragma once

#include "core/allocators/bitmap.hpp"

#include "render/next/device.hpp"

#include <wil/resource.h>

namespace sm::render::next {
    class Fence;
    class CommandBufferSet;

    class DescriptorPool {
        UINT mDescriptorSize;
        bool mIsShaderVisible;

        sm::BitMapIndexAllocator mAllocator;
        Object<ID3D12DescriptorHeap> mHeap;

        D3D12_CPU_DESCRIPTOR_HANDLE mFirstHostHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE mFirstDeviceHandle;

    public:
        DescriptorPool(CoreDevice& device, UINT size, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible) throws(RenderException);

        UINT getDescriptorSize() const noexcept { return mDescriptorSize; }
        bool isShaderVisible() const noexcept { return mIsShaderVisible; }
        ID3D12DescriptorHeap *get() noexcept { return mHeap.get(); }

        D3D12_CPU_DESCRIPTOR_HANDLE getFirstHostHandle() const noexcept { return mFirstHostHandle; }
        D3D12_GPU_DESCRIPTOR_HANDLE getFirstDeviceHandle() const noexcept { return mFirstDeviceHandle; }

        D3D12_CPU_DESCRIPTOR_HANDLE host(size_t index) const noexcept;
        D3D12_GPU_DESCRIPTOR_HANDLE device(size_t index) const noexcept;

        size_t allocate();
        D3D12_CPU_DESCRIPTOR_HANDLE allocateHost();
        D3D12_GPU_DESCRIPTOR_HANDLE allocateDevice();

        void free(size_t index);
        void free(D3D12_GPU_DESCRIPTOR_HANDLE handle);
        void free(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    };

    class Fence {
        static wil::unique_handle newEvent(LPCSTR name);

        Object<ID3D12Fence> mFence;
        wil::unique_handle mEvent;
    public:
        Fence(CoreDevice& device, uint64_t initialValue = 0, const char *name = "Fence") throws(RenderException);

        uint64_t completedValue() const;
        void waitForCompetedValue(uint64_t value);
        void waitForEvent(uint64_t value);
        void signal(ID3D12CommandQueue *queue, uint64_t value);

        ID3D12Fence *get() noexcept { return mFence.get(); }
    };

    /// @brief A command list thats buffered per frame.
    class CommandBufferSet {
        UINT mCurrentBuffer;
        std::vector<Object<ID3D12CommandAllocator>> mAllocators;
        Object<ID3D12GraphicsCommandList> mCommandList;

        ID3D12CommandAllocator *currentAllocator() noexcept { return mAllocators[mCurrentBuffer].get(); }

    public:
        CommandBufferSet(CoreDevice& device, D3D12_COMMAND_LIST_TYPE type, UINT frameCount, UINT first = 0) throws(RenderException);

        void reset(UINT index);

        ID3D12GraphicsCommandList *close();

        ID3D12GraphicsCommandList *get() noexcept { return mCommandList.get(); }
        ID3D12GraphicsCommandList *operator->() noexcept { return get(); }
    };

    void syncDeviceTimeline(ID3D12CommandQueue *dependent, ID3D12CommandQueue *independent, ID3D12Fence *fence, uint64_t value);
}
