#include "common.hpp"

#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

static render::next::SurfaceInfo newSurfaceInfo(math::uint2 size) {
    return render::next::SurfaceInfo {
        .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .size = size,
        .length = 2,
        .clear = math::float4(0.0f, 0.0f, 0.0f, 1.0f),
    };
}

LRESULT GuiWindowEvents::event(system::Window& window, UINT message, WPARAM wparam, LPARAM lparam) {
    return ImGui_ImplWin32_WndProcHandler(window.getHandle(), message, wparam, lparam);
}

void GuiWindowEvents::resize(system::Window& window, math::int2 size) {
    if (mContext == nullptr)
        return;

    mContext->updateSwapChain(newSurfaceInfo(size));
}

static system::WindowConfig newWindowConfig(const char *title) {
    return system::WindowConfig {
        .mode = system::WindowMode::eWindowed,
        .width = 800,
        .height = 600,
        .title = title,
    };
}

static render::next::ContextConfig newContextConfig(render::next::ISwapChainFactory *factory, math::uint2 size) {
    return render::next::ContextConfig {
        .autoSearchBehaviour = render::AdapterPreference::eMinimumPower,
        .swapChainFactory = factory,
        .swapChainInfo = newSurfaceInfo(size),
    };
}

GuiWindow::GuiWindow(const char *title)
    : mWindow(newWindowConfig(title), mEvents)
    , mSwapChain(mWindow.getHandle())
    , mContext(newContextConfig(&mSwapChain, mWindow.getClientSize()), mWindow.getHandle(), math::uint2 { 800, 600 })
{
    mWindow.showWindow(system::ShowWindow::eShow);
    mEvents.setContext(&mContext);
}

bool GuiWindow::next() {
    MSG msg = {};
    bool done = false;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        if (msg.message == WM_QUIT) {
            done = true;
        }
    }

    if (done) return false;

    mContext.begin();

    return true;
}

void GuiWindow::present() {
    mContext.end();
}
