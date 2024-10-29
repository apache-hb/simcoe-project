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

    math::uint2 vic20Size { VIC20_SCREEN_WIDTH, VIC20_SCREEN_HEIGHT };
    DrawContext context{config, window.getHandle(), vic20Size};
    events.context = &context;

    static const char *kVic20ColourNames[VIC20_PALETTE_SIZE] = {
        [VIC20_COLOUR_BLACK] = "Black",
        [VIC20_COLOUR_WHITE] = "White",
        [VIC20_COLOUR_DARK_BROWN] = "Dark Brown",
        [VIC20_COLOUR_LIGHT_BROWN] = "Light Brown",

        [VIC20_COLOUR_MAROON] = "Maroon",
        [VIC20_COLOUR_LIGHT_RED] = "Light Red",
        [VIC20_COLOUR_BLUE] = "Blue",
        [VIC20_COLOUR_LIGHT_BLUE] = "Light Blue",

        [VIC20_COLOUR_PURPLE] = "Purple",
        [VIC20_COLOUR_PINK] = "Pink",
        [VIC20_COLOUR_GREEN] = "Green",
        [VIC20_COLOUR_LIGHT_GREEN] = "Light Green",

        [VIC20_COLOUR_DARK_PURPLE] = "Dark Purple",
        [VIC20_COLOUR_LIGHT_PURPLE] = "Light Purple",
        [VIC20_COLOUR_YELLOW_GREEN] = "Yellow Green",
        [VIC20_COLOUR_LIGHT_YELLOW] = "Light Yellow",
    };

    // // draw a square in the middle of the screen
    // uint8_t x = VIC20_SCREEN_WIDTH / 2;
    // uint8_t y = VIC20_SCREEN_HEIGHT / 2;
    int sizeWidth = 50;
    int sizeHeight = 50;

    // context.write(100, 100, VIC20_COLOUR_LIGHT_YELLOW);

    // for (int i = 0; i < sizeWidth; ++i) {
    //     for (int j = 0; j < sizeHeight; ++j) {
    //         context.write(x + i, y + j, VIC20_COLOUR_BLUE);
    //     }
    // }

    while (nextMessage()) {
        context.begin();

        ImGui::ShowDemoWindow();

        if (ImGui::Begin("Poke")) {
            static uint8_t x = 0;
            static uint8_t y = 0;
            static int colour = VIC20_COLOUR_BLACK;

            ImGui::InputScalar("X", ImGuiDataType_U8, &x);
            ImGui::InputScalar("Y", ImGuiDataType_U8, &y);
            if (ImGui::BeginCombo("Colour", kVic20ColourNames[colour])) {
                for (int i = 0; i < VIC20_PALETTE_SIZE; ++i) {
                    if (ImGui::Selectable(kVic20ColourNames[i])) {
                        colour = i;
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("Poke")) {
                context.write(x, y, colour);

                for (int i = 0; i < sizeWidth; ++i) {
                    for (int j = 0; j < sizeHeight; ++j) {
                        context.write(x + i, y + j, VIC20_COLOUR_BLUE);
                    }
                }
            }
        }
        ImGui::End();

        if (ImGui::Begin("VIC20")) {
            D3D12_GPU_DESCRIPTOR_HANDLE srv = context.getVic20TargetSrv();
            ImGui::Image(reinterpret_cast<ImTextureID>(srv.ptr), ImVec2(vic20Size.width, vic20Size.height));
        }
        ImGui::End();

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
