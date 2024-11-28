#include "stdafx.hpp"

#include "archive/fs.hpp"
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

static constexpr math::uint2 kNtscSize { 486, 440 };

class ClientWindow final : public launch::AppWindow {
    Vic20DrawContext mContext;

    void begin() override { mContext.begin(); }
    void end() override { mContext.end(); }
    render::next::CoreContext& getContext() override { return mContext; }

public:
    ClientWindow(const std::string& title, db::Connection *db)
        : AppWindow(title, db)
        , mContext(newContextConfig(), mWindow.getHandle(), kNtscSize)
    {
        initWindow();
    }

    Vic20DrawContext& getVic20Context() { return mContext; }
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

class ScreenMemory {
    uint8_t mScreenColour[VIC20_SCREEN_CHARS_WIDTH * VIC20_SCREEN_CHARS_HEIGHT];
    uint8_t mScreenCharacter[VIC20_SCREEN_CHARS_WIDTH * VIC20_SCREEN_CHARS_HEIGHT];

public:
    ScreenMemory() = default;

    void load(const fs::path& path, Vic20DrawContext& context) {
        memset(mScreenColour, 0, sizeof(mScreenColour));
        memset(mScreenCharacter, 0, sizeof(mScreenCharacter));

        std::vector<uint8_t> screen = readFileBytes<uint8_t>(path);
        if (screen.size() != sizeof(mScreenColour) + sizeof(mScreenCharacter)) {
            throw std::runtime_error(fmt::format("invalid screen memory map size, must be {} bytes long", sizeof(mScreenColour) + sizeof(mScreenCharacter)));
        }
        memcpy(mScreenColour, screen.data(), sizeof(mScreenColour));
        memcpy(mScreenCharacter, screen.data() + sizeof(mScreenColour), sizeof(mScreenCharacter));

        for (uint8_t x = 0; x < VIC20_SCREEN_CHARS_WIDTH; x++) {
            for (uint8_t y = 0; y < VIC20_SCREEN_CHARS_HEIGHT; y++) {
                uint16_t index = x * VIC20_SCREEN_CHARS_HEIGHT + y;
                context.write(x, y, mScreenColour[index], mScreenCharacter[index]);
            }
        }
    }

    void save(const fs::path& path) {
        std::ofstream file(path, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char *>(mScreenColour), sizeof(mScreenColour));
            file.write(reinterpret_cast<const char *>(mScreenCharacter), sizeof(mScreenCharacter));
        } else {
            throw std::runtime_error("failed to open file: " + path.string());
        }
    }

    void write(Vic20DrawContext& context, uint8_t x, uint8_t y, uint8_t colour, uint8_t character) {
        uint16_t index = x * VIC20_SCREEN_CHARS_HEIGHT + y;
        mScreenColour[index] = colour;
        mScreenCharacter[index] = character;
        context.write(x, y, colour, character);
    }

    uint8_t characterAt(uint8_t x, uint8_t y) const {
        return mScreenCharacter[x * VIC20_SCREEN_CHARS_HEIGHT + y];
    }
};

struct ScreenPixel {
    uint8_t colour;
    uint8_t character;
};

void DrawCharacterEditor(draw::shared::Vic20Character& character, int fg, int bg) {
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
}

static void DrawMemoryMapScreen(bool& showScreenMemoryMap, bool& showCharmapIndex, size_t& selected, ScreenMemory& screen, Vic20DrawContext& context) {
    if (!showScreenMemoryMap) return;

    if (ImGui::Begin("Screen Memory Map", &showScreenMemoryMap)) {
        ImGui::Checkbox("Show Charmap Index", &showCharmapIndex);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));

        int size = VIC20_SCREEN_CHARS_WIDTH * VIC20_SCREEN_CHARS_HEIGHT;
        for (int i = 0; i < size; i++) {
            int x = i / VIC20_SCREEN_CHARS_HEIGHT;
            int y = i % VIC20_SCREEN_CHARS_HEIGHT;

            ImGui::PushID(i);

            char label[32];
            if (showCharmapIndex) {
                (void)snprintf(label, sizeof(label), "%d##pixel", screen.characterAt(x, y));
            } else {
                (void)snprintf(label, sizeof(label), "%d##pixel", i);
            }
            if (ImGui::Button(label, ImVec2(30, 22))) {
                // do something
                selected = screen.characterAt(x, y);
            }

            ImGui::PopID();

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("CHARACTER_MAP_INDEX")) {
                    ScreenPixel pixel = *reinterpret_cast<const ScreenPixel *>(payload->Data);
                    screen.write(context, x, y, pixel.colour, pixel.character);
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

static void DrawCharacterMap(bool showCharacterMap, CharacterMap& charmap, size_t& selected) {
    if (!showCharacterMap) return;

    auto title = fmt::format("Character Map - {}##CharacterMap", charmap.name());
    if (ImGui::Begin(title.c_str(), &showCharacterMap)) {
        ImGui::Text("Path: %s", charmap.path());
        ImGui::Text("%zu characters (%zu bytes)", charmap.characters().size(), charmap.bytes().size());

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

            ImGui::Text("Selected: %zu", selected);

            PaletteColourPicker("Background", &bg);
            PaletteColourPicker("Foreground", &fg);

            std::span<draw::shared::Vic20Character> characters = charmap.characters();
            draw::shared::Vic20Character& character = characters[selected];

            DrawCharacterEditor(character, fg, bg);

            ImGui::EndChild();
        }
    }

    ImGui::End();

}

struct CharacterMapWidget {
    CharacterMap charmap;
    bool open = true;
    size_t selected = 0;

    void Draw() {
        DrawCharacterMap(open, charmap, selected);
    }
};

struct ScreenMemoryWidget {
    ScreenMemory screen;
    bool open = true;
    bool showCharmapIndex = false;

    void Draw(size_t& selected, Vic20DrawContext& context) {
        DrawMemoryMapScreen(open, showCharmapIndex, selected, screen, context);
    }
};

static int commonMain() noexcept try {
    db::Environment env = db::Environment::create(db::DbType::eSqlite3);
    db::Connection db = env.connect({ .host = "client.db" });

    ClientWindow clientWindow{"VIC20", &db};
    Vic20DrawContext& context = clientWindow.getVic20Context();

    ImGui::FileBrowser openCharacterMapRom{ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter};
    openCharacterMapRom.SetInputName("Open Character Map ROM");

    ImGui::FileBrowser saveCharacterMapRom{
        ImGuiFileBrowserFlags_CreateNewDir |
        ImGuiFileBrowserFlags_CloseOnEsc |
        ImGuiFileBrowserFlags_EnterNewFilename |
        ImGuiFileBrowserFlags_ConfirmOnEnter
    };
    saveCharacterMapRom.SetInputName("Save Character Map ROM");

    ImGui::FileBrowser openScreenMemoryMap{ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter};
    openScreenMemoryMap.SetInputName("Open Screen Memory Map");

    ImGui::FileBrowser saveScreenMemoryMap{
        ImGuiFileBrowserFlags_CreateNewDir |
        ImGuiFileBrowserFlags_CloseOnEsc |
        ImGuiFileBrowserFlags_EnterNewFilename |
        ImGuiFileBrowserFlags_ConfirmOnEnter
    };
    saveScreenMemoryMap.SetInputName("Save Screen Memory Map");

    char userprofile[MAX_PATH];
    GetEnvironmentVariableA("USERPROFILE", userprofile, MAX_PATH);

    openCharacterMapRom.SetPwd(userprofile);
    saveCharacterMapRom.SetPwd(userprofile);
    openScreenMemoryMap.SetPwd(userprofile);
    saveScreenMemoryMap.SetPwd(userprofile);

    CharacterMapWidget charmap;
    ScreenMemoryWidget screen;
    std::string error;
    bool showDemoWindow = false;

    while (clientWindow.next()) {
        openCharacterMapRom.Display();
        saveCharacterMapRom.Display();
        openScreenMemoryMap.Display();
        saveScreenMemoryMap.Display();

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

                if (ImGui::MenuItem("New Character Map ROM")) {
                    ImGui::OpenPopup("New Character Map ROM");
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Open Screen Memory Map")) {
                    openScreenMemoryMap.Open();
                }

                if (ImGui::MenuItem("Save Screen Memory Map")) {
                    saveScreenMemoryMap.Open();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                ImGui::MenuItem("Character Map", nullptr, &charmap.open, charmap.charmap.bytes().size() > 0);
                ImGui::MenuItem("Screen Memory Map", nullptr, &screen.open);
                ImGui::MenuItem("Demo Window", nullptr, &showDemoWindow);
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (openCharacterMapRom.HasSelected()) {
            try {
                charmap.charmap.load(openCharacterMapRom.GetSelected());
                std::span<draw::shared::Vic20Character> characters = charmap.charmap.characters();
                for (size_t i = 0; i < characters.size(); i++) {
                    context.writeCharacter(i, characters[i]);
                }
                charmap.open = true;
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            openCharacterMapRom.ClearSelected();
        }

        if (saveCharacterMapRom.HasSelected()) {
            try {
                charmap.charmap.save(saveCharacterMapRom.GetSelected());
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            saveCharacterMapRom.ClearSelected();
        }

        if (openScreenMemoryMap.HasSelected()) {
            try {
                screen.screen.load(openScreenMemoryMap.GetSelected(), context);
                screen.open = true;
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            openScreenMemoryMap.ClearSelected();
        }

        if (saveScreenMemoryMap.HasSelected()) {
            try {
                screen.screen.save(saveScreenMemoryMap.GetSelected());
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            saveScreenMemoryMap.ClearSelected();
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

        charmap.Draw();
        screen.Draw(charmap.selected, context);

        if (ImGui::Begin("Poke")) {
            static int first = 0;
            bool reload = false;

            std::span<std::byte> charmapBytes = charmap.charmap.bytes();
            std::span<draw::shared::Vic20Character> characters = charmap.charmap.characters();

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
                screen.screen.write(context, x, y, VIC20_CHAR_COLOUR(fg, bg), c);
            }

            if (ImGui::Button("Draw character map")) {
                int index = 0;
                for (int i = 0; i < VIC20_SCREEN_CHARS_WIDTH; i++) {
                    for (int j = 0; j < VIC20_SCREEN_CHARS_HEIGHT; j++) {
                        screen.screen.write(context, i, j, VIC20_CHAR_COLOUR(fg, bg), index++);
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
                        screen.screen.write(context, i, j, VIC20_CHAR_COLOUR(fg, bg), c);
                    }
                }
            }

            static char buffer[256] = {0};
            ImGui::InputText("Text", buffer, sizeof(buffer));

            if (ImGui::Button("Draw text")) {
                for (size_t i = 0; i < strlen(buffer); i++) {
                    screen.screen.write(context, x + i, y, VIC20_CHAR_COLOUR(fg, bg), buffer[i]);
                }
            }
        }
        ImGui::End();

        if (ImGui::Begin("VIC20")) {
            D3D12_GPU_DESCRIPTOR_HANDLE srv = context.getVic20TargetSrv();
            ImGui::Image(std::bit_cast<ImTextureID>(srv), ImVec2(kNtscSize.width, kNtscSize.height));
        }
        ImGui::End();

        clientWindow.present();
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
