#include "render/next/context/core.hpp"
#include "draw/next/context.hpp"

#include "render/next/surface/swapchain.hpp"
#include "system/system.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

using namespace sm;
using namespace sm::render::next;
using namespace sm::draw::next;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

class WindowEvents final : public system::IWindowEvents {
    LRESULT event(system::Window& window, UINT message, WPARAM wparam, LPARAM lparam) override {
        return ImGui_ImplWin32_WndProcHandler(window.get_handle(), message, wparam, lparam);
    }

    void resize(system::Window& window, math::int2 size) override {
        if (context != nullptr) {
            SurfaceInfo info {
                .format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .size = math::uint2(size),
                .length = 2,
            };

            context->updateSwapChain(info);
        }
    }

public:
    CoreContext *context = nullptr;
};

static bool nextMessage() {
    MSG msg = {};
    bool done = false;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
        if (msg.message == WM_QUIT) {
            done = true;
        }
    }

    return !done;
}

int main(int argc, const char **argv) noexcept try {
    system::create(GetModuleHandle(nullptr));
    WindowEvents events{};
    system::WindowConfig windowConfig {
        .mode = system::WindowMode::eWindowed,
        .width = 1280,
        .height = 720,
        .title = "Dear ImGui",
    };

    system::Window window{windowConfig, events};
    window.showWindow(system::ShowWindow::eShow);

    WindowSwapChainFactory hwndSwapChain{window.getHandle()};

    ContextConfig config {
        // .flags = DebugFlags::eDeviceDebugLayer
        //        | DebugFlags::eFactoryDebug
        //        | DebugFlags::eDeviceRemovedInfo
        //        | DebugFlags::eInfoQueue
        //        | DebugFlags::eAutoName
        //        | DebugFlags::eGpuValidation
        //        | DebugFlags::eDirectStorageDebug
        //        | DebugFlags::eDirectStorageBreak
        //        | DebugFlags::eDirectStorageNames
        //        | DebugFlags::eWinPixEventRuntime,
        .targetLevel = FeatureLevel::eLevel_11_0,
        .swapChainFactory = &hwndSwapChain,
        .swapChainInfo = {
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .size = window.getClientCoords().size(),
            .length = 2,
        },
    };

    DrawContext context{config, window.getHandle()};
    events.context = &context;

    while (nextMessage()) {
        context.begin();

        ImGui::ShowDemoWindow();

        context.end();
    }
} catch (const render::RenderException& e) {
    LOG_ERROR(GlobalLog, "render error: {}", e.what());
    return -1;
} catch (const std::exception& e) {
    LOG_ERROR(GlobalLog, "unhandled exception: {}", e.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception");
    return -1;
}
