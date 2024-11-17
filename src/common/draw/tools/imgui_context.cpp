#include "draw/next/vic20.hpp"

#include "render/next/surface/swapchain.hpp"
#include "system/system.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

#include <imfilebrowser.h>

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
        io_printf(io, "System error detected: (%s)\n", error.what());
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

template<typename T>
static std::vector<T> readFileBytes(const sm::fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + path.string());
    }

    file.seekg(0, std::ios::end);
    size_t sizeInBytes = file.tellg();
    size_t size = sizeInBytes / sizeof(T);

    std::vector<T> data(size);
    file.seekg(0, std::ios::beg);

    file.read(reinterpret_cast<char *>(data.data()), size * sizeof(T));

    return data;
}

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

static void PaletteColourPicker(const char *label, int *colour)
{
    if (ImGui::BeginCombo(label, kVic20ColourNames[*colour]))
    {
        for (int i = 0; i < VIC20_PALETTE_SIZE; ++i)
        {
            ImGui::PushID(i);
            // draw a little square with the colour of this choice
            // on the left side of the selectable as a visual preview

            math::float3 palette = draw::shared::kVic20Palette[i];
            ImGui::ColorButton("##colour", ImVec4(palette.r, palette.g, palette.b, 1.0f), ImGuiColorEditFlags_NoPicker);

            ImGui::PopID();

            ImGui::SameLine();

            if (ImGui::Selectable(kVic20ColourNames[i]))
            {
                *colour = i;
            }

            if (i == *colour)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}

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

    // math::uint2 ntscSize { 486, 440 };

    math::uint2 vic20Size { VIC20_SCREEN_WIDTH, VIC20_SCREEN_HEIGHT };
    Vic20DrawContext context{config, window.getHandle(), vic20Size};
    events.context = &context;

    // context.write(
    //     0, 0,
    //     VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLUE), 99
    // );

    // context.write(
    //     VIC20_SCREEN_CHARS_WIDTH - 1, 0,
    //     VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLUE), 99
    // );

    context.write(
        0, 11,
        VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLUE), 99
    );

    context.write(
        0, 12,
        VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_GREEN), 99
    );

    context.write(
        0, 13,
        VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_LIGHT_RED), 99
    );

    // context.write(
    //     0, VIC20_SCREEN_CHARS_HEIGHT - 1,
    //     VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLUE), 99
    // );

    // context.write(
    //     VIC20_SCREEN_CHARS_WIDTH - 1, VIC20_SCREEN_CHARS_HEIGHT - 1,
    //     VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLUE), 99
    // );

    ImGui::FileBrowser openCharacterMapRom{ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter};
    openCharacterMapRom.SetInputName("Open Character Map ROM");

    char userprofile[MAX_PATH];
    GetEnvironmentVariableA("USERPROFILE", userprofile, MAX_PATH);

    openCharacterMapRom.SetPwd(userprofile);

    std::vector<uint8_t> charmapBytes;
    fs::path selected;
    std::string error;

    while (nextMessage()) {
        context.begin();

        openCharacterMapRom.Display();

        ImGui::DockSpaceOverViewport();
        if (ImGui::BeginMainMenuBar()) {
            ImGui::TextUnformatted("VIC20");
            ImGui::Separator();

            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open Character Map ROM")) {
                    openCharacterMapRom.Open();
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (openCharacterMapRom.HasSelected()) {
            selected = openCharacterMapRom.GetSelected();

            try {
                charmapBytes = readFileBytes<uint8_t>(selected);
                draw::shared::Vic20Character *data = reinterpret_cast<draw::shared::Vic20Character *>(charmapBytes.data());
                size_t size = charmapBytes.size() / sizeof(draw::shared::Vic20Character);
                for (size_t i = 0; i < size; i++) {
                    context.writeCharacter(i, data[i]);
                }
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            openCharacterMapRom.ClearSelected();
        }

        if (ImGui::BeginPopupModal("Error")) {
            ImGui::TextUnformatted(error.c_str());
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (ImGui::Begin("Character Map")) {
#if 0
            ImGui::Text("Character Map %s - %zu characters", selected.string().c_str(), charmap.size());
            static int selectedChar = 0;
            ImGui::InputScalar("Character", ImGuiDataType_S32, &selectedChar);

            if (!charmapBytes.empty()) {
                selectedChar = std::clamp(selectedChar, 0, int(charmap.size() - 1));

                draw::shared::Vic20Character character = charmap[selectedChar];
                // byte order is reversed
                for (int i = 0; i < 8; i++) {
                    char buffer[9] = {0};
                    for (int j = 0; j < 8; j++) {
                        buffer[(8 - j) - 1] = (character.data & (1ull << (i * 8 + j))) ? '#' : '.';
                    }

                    ImGui::TextUnformatted(buffer);
                }
            }
#endif
        }
        ImGui::End();

        ImGui::ShowDemoWindow();

        if (ImGui::Begin("Poke")) {
            static int slide = 0;
            static int first = 0;
            bool reload = false;

            reload |= ImGui::SliderInt("Slide", &slide, 0, sizeof(draw::shared::Vic20Character) - 1);
            reload |= ImGui::SliderInt("First", &first, 0, (charmapBytes.size() / sizeof(draw::shared::Vic20Character) - 256));
            reload |= ImGui::Button("Reload charmap");

            if (reload) {
                draw::shared::Vic20Character *data = reinterpret_cast<draw::shared::Vic20Character *>(charmapBytes.data() + slide);
                size_t size = (charmapBytes.size() - slide) / sizeof(draw::shared::Vic20Character);
                for (size_t i = 0; i < size; i++) {
                    context.writeCharacter(i, data[i + first]);
                }
            }

            static int x = 0;
            static int y = 0;
            static int c = 0;
            static int fg = VIC20_COLOUR_WHITE;
            static int bg = VIC20_COLOUR_BLACK;

            ImGui::SliderInt("X", &x, 0, VIC20_SCREEN_CHARS_WIDTH - 1);
            ImGui::SliderInt("Y", &y, 0, VIC20_SCREEN_CHARS_HEIGHT - 1);
            ImGui::SliderInt("Character", &c, 0, 255);

            PaletteColourPicker("Background", &bg);
            PaletteColourPicker("Foreground", &fg);

            if (ImGui::Button("Poke")) {
                context.write(x, y, VIC20_CHAR_COLOUR(fg, bg), c);
            }

            if (ImGui::Button("Draw character map")) {
                int index = 0;
                for (int i = 0; i < VIC20_SCREEN_CHARS_WIDTH; i++) {
                    for (int j = 0; j < VIC20_SCREEN_CHARS_HEIGHT; j++) {
                        context.write(i, j, VIC20_CHAR_COLOUR(fg, bg), index++);
                    }
                }
            }

            static bool fill = false;
            ImGui::Checkbox("Fill", &fill);
            ImGui::SameLine();
            ImGui::SliderInt("Index", &c, 0, 255);

            if (fill) {
                for (int i = 0; i < VIC20_SCREEN_CHARS_WIDTH; i++) {
                    for (int j = 0; j < VIC20_SCREEN_CHARS_HEIGHT; j++) {
                        context.write(i, j, VIC20_CHAR_COLOUR(fg, bg), c);
                    }
                }
            }

            static char buffer[256] = {0};
            ImGui::InputText("Text", buffer, sizeof(buffer));

            if (ImGui::Button("Draw text")) {
                for (size_t i = 0; i < strlen(buffer); i++) {
                    context.write(x + i, y, VIC20_CHAR_COLOUR(fg, bg), buffer[i]);
                }
            }
        }
        ImGui::End();

        if (ImGui::Begin("VIC20")) {
            D3D12_GPU_DESCRIPTOR_HANDLE srv = context.getVic20TargetSrv();
            ImGui::Image(std::bit_cast<ImTextureID>(srv), ImVec2(vic20Size.width, vic20Size.height));
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
