#pragma once

#include "render/next/context/core.hpp"

namespace sm::draw::next {
    using IContextResource = render::next::IContextResource;
    using CoreContext = render::next::CoreContext;
    using SurfaceInfo = render::next::SurfaceInfo;

    class ImGuiDrawContext final : public IContextResource {
        HWND mWindow;
    public:
        ImGuiDrawContext(CoreContext& ctx, HWND hwnd) noexcept
            : IContextResource(ctx)
            , mWindow(hwnd)
        { }

        void setup();
        void destroy() noexcept;
        void begin();
        void end(ID3D12GraphicsCommandList *list);

        void setupPlatform();
        void setupRender(ID3D12Device *device, DXGI_FORMAT format, UINT frames, render::next::DescriptorPool& srvHeap, size_t srvHeapIndex);

        // IContextResource
        void reset() override;
        void create() override;
        void update(SurfaceInfo info) override;
        // ~IContextResource
    };

    class DrawContext : public render::next::CoreContext {
        using Super = render::next::CoreContext;

        ImGuiDrawContext *mImGui;

    public:
        DrawContext(render::next::ContextConfig config, HWND hwnd);
        ~DrawContext() noexcept;

        void begin();
        void end();
    };
}
