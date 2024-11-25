#include "stdafx.hpp"

#include "archive/fs.hpp"
#include "launch/launch.hpp"

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

            ImGui::ColorButton("##colour", float4(draw::shared::kVic20Palette[i], 1.f), ImGuiColorEditFlags_NoPicker);

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

class CharacterMap {
    std::vector<std::byte> mData;
    std::string mPath;
    std::string mName;

public:
    CharacterMap() = default;

    CharacterMap(const fs::path& path) {
        load(path);
    }

    void load(const fs::path& path) {
        mData = readFileBytes<std::byte>(path);
        mPath = path.string();
        mName = path.filename().string();
    }

    std::span<draw::shared::Vic20Character> characters() {
        return std::span<draw::shared::Vic20Character>(
            reinterpret_cast<draw::shared::Vic20Character *>(mData.data()),
            mData.size() / sizeof(draw::shared::Vic20Character)
        );
    }

    std::span<std::byte> bytes() {
        return std::span<std::byte>(mData);
    }

    const char *path() const {
        return mPath.c_str();
    }

    const char *name() const {
        return mName.c_str();
    }

    void save(const fs::path& path) {
        std::ofstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char *>(mData.data()), mData.size());
        } else {
            throw std::runtime_error("failed to open file: " + path.string());
        }
    }
};

struct ScreenPixel {
    uint8_t colour;
    uint8_t character;
};

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
        .targetLevel = FeatureLevel::eLevel_11_0,
        .swapChainFactory = &hwndSwapChain,
        .swapChainInfo = {
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .size = window.getClientCoords().size(),
            .length = 2,
        },
    };

    math::uint2 ntscSize { 486, 440 };
    Vic20DrawContext context{config, window.getHandle(), ntscSize};
    events.context = &context;

    ImGui::FileBrowser openCharacterMapRom{ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter};
    openCharacterMapRom.SetInputName("Open Character Map ROM");

    ImGui::FileBrowser saveCharacterMapRom{
        ImGuiFileBrowserFlags_CreateNewDir |
        ImGuiFileBrowserFlags_CloseOnEsc |
        ImGuiFileBrowserFlags_EnterNewFilename |
        ImGuiFileBrowserFlags_ConfirmOnEnter
    };
    saveCharacterMapRom.SetInputName("Save Character Map ROM");

    char userprofile[MAX_PATH];
    GetEnvironmentVariableA("USERPROFILE", userprofile, MAX_PATH);

    openCharacterMapRom.SetPwd(userprofile);
    saveCharacterMapRom.SetPwd(userprofile);

    CharacterMap charmap;
    std::string error;
    bool showCharacterMap = false;
    bool showDemoWindow = false;
    bool showScreenMemoryMap = false;

    while (nextMessage()) {
        context.begin();

        openCharacterMapRom.Display();
        saveCharacterMapRom.Display();

        ImGui::DockSpaceOverViewport();
        if (ImGui::BeginMainMenuBar()) {
            ImGui::TextUnformatted("VIC20");
            ImGui::Separator();

            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open Character Map ROM")) {
                    openCharacterMapRom.Open();
                }

                if (ImGui::MenuItem("Save Character Map ROM")) {
                    saveCharacterMapRom.Open();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Character Map", nullptr, &showCharacterMap, charmap.bytes().size() > 0);
                ImGui::MenuItem("Screen Memory Map", nullptr, &showScreenMemoryMap);
                ImGui::MenuItem("Demo Window", nullptr, &showDemoWindow);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (openCharacterMapRom.HasSelected()) {
            try {
                charmap.load(openCharacterMapRom.GetSelected());
                std::span<draw::shared::Vic20Character> characters = charmap.characters();
                for (size_t i = 0; i < characters.size(); i++) {
                    context.writeCharacter(i, characters[i]);
                }
                showCharacterMap = true;
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            openCharacterMapRom.ClearSelected();
        }

        if (saveCharacterMapRom.HasSelected()) {
            try {
                charmap.save(saveCharacterMapRom.GetSelected());
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            saveCharacterMapRom.ClearSelected();
        }

        if (ImGui::BeginPopupModal("Error")) {
            ImGui::TextUnformatted(error.c_str());
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (showDemoWindow) {
            ImGui::ShowDemoWindow(&showDemoWindow);
        }

        if (showCharacterMap) {
            auto title = fmt::format("Character Map - {}##CharacterMap", charmap.name());
            if (ImGui::Begin(title.c_str(), &showCharacterMap)) {
                ImGui::Text("Path: %s", charmap.path());
                ImGui::Text("%zu characters (%zu bytes)", charmap.characters().size(), charmap.bytes().size());

                static size_t selected = 0;

                {
                    ImVec2 avail = ImGui::GetContentRegionAvail();
                    ImGui::BeginChild("ChildL", ImVec2(avail.x * 0.5f, avail.y));

                    ImGuiStyle& style = ImGui::GetStyle();
                    float2 size = { 40, 40 };
                    float windowVisibleX2 = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
                    // draw grid
                    for (size_t i = 0; i < charmap.characters().size(); i++) {
                        ImGui::PushID(i);
                        if (ImGui::Button(fmt::format("{}", i).c_str(), size)) {
                            selected = i;
                        }

                        if (ImGui::BeginDragDropSource()) {
                            ScreenPixel pixel = {
                                .colour = VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK),
                                .character = uint8_t(i),
                            };
                            ImGui::SetDragDropPayload("CHARACTER_MAP_INDEX", &pixel, sizeof(ScreenPixel));
                            ImGui::EndDragDropSource();
                        }

                        float lastButtonX2 = ImGui::GetItemRectMax().x;
                        float nextButtonX2 = lastButtonX2 + style.ItemSpacing.x + size.x;
                        if (i + 1 < charmap.characters().size() && nextButtonX2 < windowVisibleX2) {
                            ImGui::SameLine();
                        }

                        ImGui::PopID();
                    }

                    ImGui::EndChild();
                }

                ImGui::SameLine();

                {
                    ImGui::BeginChild("ChildR");
                    static int fg = VIC20_COLOUR_WHITE;
                    static int bg = VIC20_COLOUR_BLACK;

                    PaletteColourPicker("Background", &bg);
                    PaletteColourPicker("Foreground", &fg);

                    std::span<draw::shared::Vic20Character> characters = charmap.characters();
                    draw::shared::Vic20Character& character = characters[selected];

                    // draw an 8x8 grid of buttons, with each being a pixel in the
                    // character.

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));

                    for (int i = 0; i < 8; i++) {
                        for (int j = 0; j < 8; j++) {
                            int index = i * 8 + (7 - j);

                            bool set = character.data & (1ull << index);

                            if (set) {
                                ImGui::PushStyleColor(ImGuiCol_Button, float4(draw::shared::kVic20Palette[fg], 1.f));
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Button, float4(draw::shared::kVic20Palette[bg], 1.f));
                            }

                            ImGui::PushID(index);
                            if (ImGui::Button("##pixel", ImVec2(20, 20))) {
                                character.data ^= (1ull << index);
                            }
                            ImGui::PopID();

                            ImGui::PopStyleColor();

                            if (j < 7) {
                                ImGui::SameLine();
                            }
                        }
                    }

                    ImGui::PopStyleVar();

                    ImGui::EndChild();
                }
            }

            ImGui::End();
        }

        if (showScreenMemoryMap) {
            if (ImGui::Begin("Screen Memory Map", &showScreenMemoryMap)) {

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));

                int size = VIC20_SCREEN_CHARS_WIDTH * VIC20_SCREEN_CHARS_HEIGHT;
                for (int i = 0; i < size; i++) {
                    ImGui::PushID(i);

                    char label[32];
                    (void)snprintf(label, sizeof(label), "%d##pixel", i);
                    if (ImGui::Button(label, ImVec2(30, 22))) {
                        // do something
                    }

                    ImGui::PopID();

                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CHARACTER_MAP_INDEX")) {
                            ScreenPixel pixel = *reinterpret_cast<const ScreenPixel *>(payload->Data);
                            int x = i / VIC20_SCREEN_CHARS_HEIGHT;
                            int y = i % VIC20_SCREEN_CHARS_HEIGHT;
                            context.write(x, y, pixel.colour, pixel.character);
                        }
                        ImGui::EndDragDropTarget();
                    }

                    if (i % VIC20_SCREEN_CHARS_WIDTH != VIC20_SCREEN_CHARS_WIDTH - 1) {
                        ImGui::SameLine();
                    }
                }

                ImGui::PopStyleVar();
            }
            ImGui::End();
        }

        if (ImGui::Begin("Poke")) {
            static int first = 0;
            bool reload = false;

            std::span<std::byte> charmapBytes = charmap.bytes();
            std::span<draw::shared::Vic20Character> characters = charmap.characters();

            reload |= ImGui::SliderInt("First", &first, 0, characters.size() - 256);
            reload |= ImGui::Button("Reload charmap");

            if (reload) {
                for (size_t i = 0; i < characters.size(); i++) {
                    context.writeCharacter(i, characters[i + first]);
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
