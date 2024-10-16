#pragma once

#include "render/next/device.hpp"

namespace sm::render::next {
    class Fence;
    class CoreContext;

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

        UINT getNextIndex() noexcept { return (mCurrentBuffer + 1) % mAllocators.size(); }
        ID3D12CommandAllocator *currentAllocator() noexcept { return mAllocators[mCurrentBuffer].get(); }

    public:
        CommandBufferSet(CoreDevice& device, D3D12_COMMAND_LIST_TYPE type, UINT frameCount) throws(RenderException);

        void reset();

        ID3D12GraphicsCommandList *close();
    };
}
