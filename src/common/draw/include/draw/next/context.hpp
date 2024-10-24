#pragma once

#include "render/next/context/core.hpp"

namespace sm::draw::next {
    class ImGuiDrawContext {

    public:
        void create();
        void destroy() noexcept;
        void begin();
        void end(ID3D12GraphicsCommandList *list);

        void setupPlatform(HWND hwnd);
        void setupRender(ID3D12Device *device, DXGI_FORMAT format, UINT frames, render::next::DescriptorPool& srvHeap, size_t srvHeapIndex);
    };

    class DrawContext : public render::next::CoreContext {
        using Super = render::next::CoreContext;

        ImGuiDrawContext mImGui;

    public:
        DrawContext(render::next::ContextConfig config, HWND hwnd);
        ~DrawContext() noexcept;

        void begin();
        void end();
    };
}
