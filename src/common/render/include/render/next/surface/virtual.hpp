#pragma once

#include "render/next/components.hpp"
#include "render/next/surface/surface.hpp"

namespace sm::render::next {
    class VirtualSwapChainFactory;
    class VirtualSwapChain;

    struct VirtualSurface {
        Object<D3D12MA::Allocation> target;
        Object<D3D12MA::Allocation> readback;

        std::span<const uint32_t> image;

        ID3D12Resource *getRenderTarget() { return target->GetResource(); }
        ID3D12Resource *getCopyTarget() { return readback->GetResource(); }
    };

    using SurfaceList = std::vector<VirtualSurface>;

    class VirtualSwapChain final : public ISwapChain {
        UINT mIndex = 0;
        SurfaceList mSurfaces;
        SurfaceInfo mSurfaceInfo;
        CoreDevice& mDevice;
        D3D12MA::Allocator *mAllocator;
        ID3D12CommandQueue *mCommandQueue;

        CommandBufferSet mCommandList;

        ID3D12Resource *getSurfaceAt(UINT index) override;
        void updateSurfaces(SurfaceInfo info) override;

        ID3D12Resource *getCopyTarget(UINT index) { return mSurfaces[index].getCopyTarget(); }

    public:
        VirtualSwapChain(VirtualSwapChainFactory *factory, SurfaceInfo info, SurfaceList surfaces, SurfaceCreateObjects objects);
        ~VirtualSwapChain() noexcept;

        UINT currentSurfaceIndex() override;
        void present(UINT sync) override;

        std::span<const uint32_t> getImage(UINT index) { return mSurfaces[index].image; }
        std::span<const uint32_t> getImage() { return mSurfaces[mIndex].image; }
        SurfaceInfo getSurfaceInfo() const { return mSurfaceInfo; }
    };

    class VirtualSwapChainFactory final : public ISwapChainFactory {
        std::map<ID3D12Device*, VirtualSwapChain*> mSwapChains;
    public:
        ISwapChain *createSwapChain(SurfaceCreateObjects objects, const SurfaceInfo& info) override;

        VirtualSwapChainFactory();

        void removeSwapChain(ID3D12Device *device);

        VirtualSwapChain *getSwapChain(ID3D12Device *device) {
            return mSwapChains.at(device);
        }
    };
}
