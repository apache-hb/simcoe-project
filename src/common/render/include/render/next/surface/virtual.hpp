#pragma once

#include "render/next/components.hpp"
#include "render/next/surface/surface.hpp"

namespace sm::render::next {
    class VirtualSwapChainFactory;
    class VirtualSwapChain;

    struct VirtualSurface {
        Object<D3D12MA::Allocation> target;
        Object<D3D12MA::Allocation> readback;

        ID3D12Resource *getTarget() { return target->GetResource(); }
    };

    using SurfaceList = std::vector<VirtualSurface>;

    class VirtualSwapChain final : public ISwapChain {
        UINT mIndex = 0;
        SurfaceList mSurfaces;
        CoreDevice& mDevice;
        D3D12MA::Allocator *mAllocator;
        ID3D12CommandQueue *mCommandQueue;

        CommandBufferSet mCommandList;

        ID3D12Resource *getSurfaceAt(UINT index) override;
        void updateSurfaces(SurfaceInfo info) override;

    public:
        VirtualSwapChain(VirtualSwapChainFactory *factory, SurfaceList surfaces, SurfaceCreateObjects objects);
        ~VirtualSwapChain() noexcept;

        UINT currentSurfaceIndex() override;
        void present(UINT sync) override;
    };

    class VirtualSwapChainFactory final : public ISwapChainFactory {
        std::map<ID3D12Device*, VirtualSwapChain*> mSwapChains;
    public:
        ISwapChain *createSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info) override;

        VirtualSwapChainFactory();

        void removeSwapChain(ID3D12Device *device);
    };
}
