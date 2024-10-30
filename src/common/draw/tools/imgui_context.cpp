#include "draw/next/vic20.hpp"

#include "render/next/surface/swapchain.hpp"
#include "system/system.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

#include "system/system.hpp"

#include "core/macros.h"
#include "core/memory.h"

#include "io/console.h"
#include "io/io.h"

#include "format/backtrace.h"
#include "format/colour.h"

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

static fmt_backtrace_t newPrintOptions(io_t *io) {
    fmt_backtrace_t print = {
        .options = {
            .arena = get_default_arena(),
            .io = io,
            .pallete = &kColourDefault,
        },
        .header = eHeadingGeneric,
        .config = eBtZeroIndexedLines,
        .project_source_path = SMC_SOURCE_DIR,
    };

    return print;
}

class DefaultSystemError final : public sm::ISystemError {
    bt_report_t *mReport = nullptr;

    void error_begin(sm::OsError error) override {
        bt_update();

        mReport = bt_report_new(get_default_arena());
        io_t *io = io_stderr();
        io_printf(io, "System error detected: (%s)\n", error.toString().c_str());
    }

    void error_frame(bt_address_t it) override {
        bt_report_add(mReport, it);
    }

    void error_end() override {
        const fmt_backtrace_t kPrintOptions = newPrintOptions(io_stderr());
        fmt_backtrace(kPrintOptions, mReport);
        os_exit(CT_EXIT_INTERNAL); // NOLINT
    }
};

static DefaultSystemError gDefaultError{};

int main(int argc, const char **argv) noexcept try {
    SetProcessAffinityMask(GetCurrentProcess(), 0b1111'1111'1111'1111);
    bt_init();
    os_init();

    // TODO: popup window for panics and system errors
    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        bt_update();
        io_t *io = io_stderr();
        arena_t *arena = get_default_arena();

        const fmt_backtrace_t kPrintOptions = newPrintOptions(io);

        auto message = sm::vformat(msg, args);

        LOG_PANIC(GlobalLog, "{}", message);

        fmt::println(stderr, "{}", message);

        bt_report_t *report = bt_report_collect(arena);
        fmt_backtrace(kPrintOptions, report);

        // TODO: block until logs are flushed

        os_exit(CT_EXIT_INTERNAL); // NOLINT
    };

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
        .flags = DebugFlags::eDeviceDebugLayer
               | DebugFlags::eFactoryDebug
               | DebugFlags::eDeviceRemovedInfo
            //    | DebugFlags::eWarpAdapter
               | DebugFlags::eInfoQueue
               | DebugFlags::eAutoName
               | DebugFlags::eDirectStorageDebug
               | DebugFlags::eDirectStorageBreak
               | DebugFlags::eDirectStorageNames
               | DebugFlags::eWinPixEventRuntime,
        .targetLevel = FeatureLevel::eLevel_11_0,
        .swapChainFactory = &hwndSwapChain,
        .swapChainInfo = {
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .size = window.getClientCoords().size(),
            .length = 2,
        },
    };

    math::uint2 vic20Size { 525, 468 };
    Vic20DrawContext context{config, window.getHandle(), vic20Size};
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

    for (int i = 0; i < VIC20_SCREEN_WIDTH; i++) {
        context.write(i, 10, VIC20_COLOUR_WHITE);
    }

    for (int i = 0; i < VIC20_SCREEN_HEIGHT; i++) {
        context.write(VIC20_SCREEN_WIDTH / 2, i, VIC20_COLOUR_BLUE);
    }

    for (int i = 0; i < VIC20_SCREEN_HEIGHT; i++) {
        context.write(13, i, VIC20_COLOUR_BLUE);
    }

    for (int i = 0; i < VIC20_SCREEN_HEIGHT; i++) {
        context.write(VIC20_SCREEN_WIDTH - 13, i, VIC20_COLOUR_BLUE);
    }

    while (nextMessage()) {
        context.begin();

        ImGui::ShowDemoWindow();

        if (ImGui::Begin("Poke")) {
            static uint8_t x = 0;
            static uint8_t y = 0;
            static int width = 50;
            static int height = 50;
            static int colour = VIC20_COLOUR_WHITE;

            ImGui::InputScalar("Width", ImGuiDataType_S32, &width);
            ImGui::InputScalar("Height", ImGuiDataType_S32, &height);

            width = std::clamp(width, 1, 50);
            height = std::clamp(height, 1, 50);

            ImGui::InputScalar("X", ImGuiDataType_U8, &x);
            ImGui::InputScalar("Y", ImGuiDataType_U8, &y);
            if (ImGui::BeginCombo("Colour", kVic20ColourNames[colour])) {
                for (int i = 0; i < VIC20_PALETTE_SIZE; ++i) {
                    ImGui::PushID(i);
                    // draw a little square with the colour of this choice
                    // on the left side of the selectable as a visual preview

                    math::float3 palette = draw::shared::kVic20Palette[i];
                    ImGui::ColorButton("##colour", ImVec4(palette.r, palette.g, palette.b, 1.0f), ImGuiColorEditFlags_NoPicker);

                    ImGui::PopID();

                    ImGui::SameLine();

                    if (ImGui::Selectable(kVic20ColourNames[i])) {
                        colour = i;
                    }

                    if (i == colour) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (ImGui::Button("Poke")) {
                for (int i = 0; i < width; ++i) {
                    for (int j = 0; j < height; ++j) {
                        context.write(x + i, y + j, colour);
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
