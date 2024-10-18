#pragma once

#include "render/next/surface/surface.hpp"

namespace sm::render::next {
    class VirtualSwapChainFactory;
    class VirtualSwapChain;

    struct VirtualSurface {
        Object<ID3D12Resource> target;
        Object<ID3D12Resource> readback;

        Object<ID3D12Resource> getTarget() { return target.clone(); }
    };

    using SurfaceList = std::vector<VirtualSurface>;

    class VirtualSwapChain final : public ISwapChain {
        UINT mIndex = 0;
        SurfaceList mSurfaces;
        ID3D12Device1 *mDevice;

        Object<ID3D12Resource> getSurfaceAt(UINT index) override;
        void updateSurfaces(SurfaceInfo info) override;

    public:
        VirtualSwapChain(VirtualSwapChainFactory *factory, SurfaceList surfaces, ID3D12Device1 *device);
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
