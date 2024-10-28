#pragma once

#include "render/next/context/core.hpp"

namespace sm::draw::next {
    class ImGuiDrawContext final : public render::next::IContextResource {
        using Super = render::next::IContextResource;
        HWND mWindow;

        bool hasViewportSupport() const noexcept;

    public:
        ImGuiDrawContext(render::next::CoreContext& ctx, HWND hwnd) noexcept
            : Super(ctx)
            , mWindow(hwnd)
        { }

        void setup() noexcept;
        void destroy() noexcept;
        void begin() noexcept;
        void end(ID3D12GraphicsCommandList *list) noexcept;

        void updatePlatformViewports(ID3D12GraphicsCommandList *list) noexcept;

        void setupPlatform() noexcept;
        void setupRender(ID3D12Device *device, DXGI_FORMAT format, UINT frames, render::next::DescriptorPool& srvHeap, size_t srvHeapIndex) noexcept;

        // IContextResource
        void reset() noexcept override;
        void create() override;
        void update(render::next::SurfaceInfo info) override;
        // ~IContextResource
    };
}
