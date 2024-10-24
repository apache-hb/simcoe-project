#pragma once

#include "render/next/context/core.hpp"

namespace sm::draw::next {
    class ImGuiDrawContext {
        HWND mWindow;
    public:
        ImGuiDrawContext(HWND hwnd) noexcept
            : mWindow(hwnd)
        { }

        void create();
        void destroy() noexcept;
        void begin();
        void end(ID3D12GraphicsCommandList *list);

        void setupPlatform();
        void setupRender(ID3D12Device *device, DXGI_FORMAT format, UINT frames, render::next::DescriptorPool& srvHeap, size_t srvHeapIndex);
        void destroyRender();
    };

    class DrawContext : public render::next::CoreContext {
        using Super = render::next::CoreContext;

        ImGuiDrawContext mImGui;

        void setupImGuiRenderState();

    public:
        DrawContext(render::next::ContextConfig config, HWND hwnd);
        ~DrawContext() noexcept;

        void begin();
        void end();

        void setAdapter(render::AdapterLUID luid);
    };
}
