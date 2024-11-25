#include "stdafx.hpp"

#include "archive/fs.hpp"
#include "config/config.hpp"
#include "launch/launch.hpp"
#include "launch/appwindow.hpp"

#include "draw/next/vic20.hpp"

#include "render/next/surface/swapchain.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

#include <imfilebrowser.h>

using namespace sm;
using namespace sm::math;
using namespace sm::render::next;
using namespace sm::draw::next;

static sm::opt<std::string> gAppDir {
    name = "appdir",
    desc = "The application directory",
    init = "./"
};

static sm::opt<std::string> gBundlePath {
    name = "bundle",
    desc = "Path to the bundle file or directory",
    init = "./bundle.tar"
};

static sm::opt<bool> gBundlePacked {
    name = "packed",
    desc = "Is the bundled packed in a tar file?",
    init = true
};

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

static sm::IFileSystem *mountArchive(bool isPacked, const fs::path &path) {
    if (isPacked) {
        return sm::mountArchive(path);
    } else {
        return sm::mountFileSystem(path);
    }
}

static int commonMain() noexcept try {
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

    math::uint2 ntscSize { 486, 440 };

    // math::uint2 vic20Size { VIC20_SCREEN_WIDTH, VIC20_SCREEN_HEIGHT };
    Vic20DrawContext context{config, window.getHandle(), ntscSize};
    events.context = &context;

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
            ImGui::Image(std::bit_cast<ImTextureID>(srv), ImVec2(ntscSize.width, ntscSize.height));
        }
        ImGui::End();

        context.end();
    }

    return 0;
} catch (const std::exception& err) {
    LOG_ERROR(GlobalLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception");
    return -1;
}

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "client-logs.db" },
    .logPath = "client.log",
};

SM_LAUNCH_MAIN("Client", commonMain, kLaunchInfo)
