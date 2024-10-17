#pragma once

#include "core/allocators/bitmap.hpp"

#include "render/next/device.hpp"

namespace sm::render::next {
    class Fence;
    class CommandBufferSet;

    class DescriptorPool {
        UINT mDescriptorSize;
        bool mIsShaderVisible;

        D3D12_CPU_DESCRIPTOR_HANDLE mFirstHostHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE mFirstDeviceHandle;

        sm::BitMapIndexAllocator mAllocator;
        Object<ID3D12DescriptorHeap> mHeap;

    public:
        DescriptorPool(CoreDevice& device, UINT size, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisible) throws(RenderException);

        UINT getDescriptorSize() const noexcept { return mDescriptorSize; }
        bool isShaderVisible() const noexcept { return mIsShaderVisible; }

        D3D12_CPU_DESCRIPTOR_HANDLE getFirstHostHandle() const noexcept { return mFirstHostHandle; }
        D3D12_GPU_DESCRIPTOR_HANDLE getFirstDeviceHandle() const noexcept { return mFirstDeviceHandle; }

        D3D12_CPU_DESCRIPTOR_HANDLE getHostDescriptorHandle(size_t index) const noexcept;
        D3D12_GPU_DESCRIPTOR_HANDLE getDeviceDescriptorHandle(size_t index) const noexcept;
    };

    class Fence {
        constexpr static auto kCloseHandle = [](HANDLE handle) { CloseHandle(handle); };
        using GuardHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(kCloseHandle)>;

        static GuardHandle newEvent(LPCSTR name);

        Object<ID3D12Fence> mFence;
        GuardHandle mEvent;
    public:
        Fence(CoreDevice& device, uint64_t initialValue = 0) throws(RenderException);

        uint64_t value() const;
        void wait(uint64_t value);

        ID3D12Fence *get() noexcept { return mFence.get(); }
    };

    /// @brief A command list thats buffered per frame.
    class CommandBufferSet {
        UINT mCurrentBuffer;
        std::vector<Object<ID3D12CommandAllocator>> mAllocators;
        Object<ID3D12GraphicsCommandList> mCommandList;

        ID3D12CommandAllocator *currentAllocator() noexcept { return mAllocators[mCurrentBuffer].get(); }

    public:
        CommandBufferSet(CoreDevice& device, D3D12_COMMAND_LIST_TYPE type, UINT frameCount) throws(RenderException);

        void reset(UINT index);

        ID3D12GraphicsCommandList *close();

        ID3D12GraphicsCommandList *operator->() noexcept { return mCommandList.get(); }
    };
}
