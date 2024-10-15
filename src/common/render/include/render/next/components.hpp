#pragma once

#include "render/next/device.hpp"

namespace sm::render::next {
    class Fence;

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
}
