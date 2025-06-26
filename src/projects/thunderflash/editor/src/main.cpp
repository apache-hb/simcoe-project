#include "stdafx.hpp"

#include "launch/launch.hpp"
#include "launch/appwindow.hpp"

#include "draw/next/vic20.hpp"

#include "system/storage.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx12.h>

#include <imfilebrowser.h>

#include <vulkan/vulkan.h>

#include "vic20.dao.hpp"

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

template<typename T>
static void writeFileBytes(const sm::fs::path& path, std::span<const T> data) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + path.string());
    }

    file.write(reinterpret_cast<const char *>(data.data()), data.size_bytes());
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

class CharacterMapDocument {
    std::vector<std::byte> mCharacterData;
    draw::shared::Vic20CharacterMap mCharacterMap = {};
    bool mSaved = false;
    fs::path mPath;
    std::string mPathString;
    std::string mName;

    void setPath(const fs::path& path) {
        mPath = path;
        mPathString = path.string();
        mName = path.filename().string();
    }

    CharacterMapDocument(std::string name, std::vector<std::byte> data)
        : mCharacterData(std::move(data))
        , mName(std::move(name))
    {
        // round up data size to next sizeof(Vic20CharacterMap)
        size_t sz = roundup(mCharacterData.size(), sizeof(draw::shared::Vic20CharacterMap));
        mCharacterData.resize(sz);
        memcpy(&mCharacterMap, mCharacterData.data(), sizeof(mCharacterMap));
    }

public:
    static CharacterMapDocument newInMemory() {
        return CharacterMapDocument { "Untitled", { } };
    }

    static CharacterMapDocument newInMemory(const CharacterMapDocument& screen) {
        return CharacterMapDocument { fmt::format("{} Copy", screen.mName), screen.mCharacterData };
    }

    static CharacterMapDocument open(const fs::path& path) {
        std::vector<std::byte> data = readFileBytes<std::byte>(path);

        CharacterMapDocument doc { path.filename().string(), data };
        doc.mSaved = true;

        doc.setPath(path);

        return doc;
    }

    void saveToDisk() {
        writeFileBytes(mPath, std::span<const std::byte>(mCharacterData));
        mSaved = true;
    }

    void saveToFile(const fs::path& path) {
        writeFileBytes(path, std::span<const std::byte>(mCharacterData));
        setPath(path);
        mSaved = true;
    }

    bool hasBackingFile() const { return !mPath.empty(); }
    bool isSavedToDisk() const { return mSaved; }
    const fs::path& path() const { return mPath; }

    draw::shared::Vic20CharacterMap& characterMap() { return mCharacterMap; }
    draw::shared::Vic20Character& characterAt(size_t index) { return characters()[index]; }
    size_t size() const { return mCharacterData.size(); }
    std::span<std::byte> bytes() { return std::span<std::byte>(mCharacterData); }

    size_t charmapCount() const { return characterCount() / VIC20_CHARMAP_SIZE; }
    size_t characterCount() const { return (mCharacterData.size() / sizeof(draw::shared::Vic20Character)); }
    std::span<draw::shared::Vic20Character> characters() {
        draw::shared::Vic20Character *characters = reinterpret_cast<draw::shared::Vic20Character *>(mCharacterData.data());
        return std::span<draw::shared::Vic20Character>(characters, characterCount());
    }

    const char *pathString() const { return mPathString.c_str(); }
    const char *nameString() const { return mName.c_str(); }
};

class ScreenDocument {
    draw::shared::Vic20Screen mScreen;
    bool mSaved = false;
    fs::path mPath;
    std::string mPathString;
    std::string mName;

    void setPath(const fs::path& path) {
        mPath = path;
        mPathString = path.string();
        mName = path.filename().string();
    }

    ScreenDocument(std::string name, draw::shared::Vic20Screen data)
        : mScreen(data)
        , mName(std::move(name))
    { }

public:
    static ScreenDocument newInMemory() {
        return ScreenDocument { "Untitled", { } };
    }

    static ScreenDocument newInMemory(const ScreenDocument& other) {
        return ScreenDocument { fmt::format("{} Copy", other.mName), other.mScreen };
    }

    static ScreenDocument open(const fs::path& path) {
        std::vector<std::byte> data = readFileBytes<std::byte>(path);
        data.resize(roundup(data.size(), sizeof(draw::shared::Vic20Screen)));

        ScreenDocument doc { path.filename().string(), *reinterpret_cast<draw::shared::Vic20Screen*>(data.data()) };
        doc.mSaved = true;

        doc.setPath(path);

        return doc;
    }

    void saveToDisk() {
        writeFileBytes(mPath, std::span<const std::byte>(reinterpret_cast<const std::byte*>(&mScreen), sizeof(mScreen)));
        mSaved = true;
    }

    void saveToFile(const fs::path& path) {
        writeFileBytes(path, std::span<const std::byte>(reinterpret_cast<const std::byte*>(&mScreen), sizeof(mScreen)));
        setPath(path);
        mSaved = true;
    }

    void write(Vic20DrawContext& context, size_t index, uint8_t character, uint8_t colour) {
        writeColour(index, colour);
        writeCharacter(index, character);
        context.setScreen(mScreen);
    }

    bool hasBackingFile() { return !mPath.empty(); }
    bool isSavedToDisk() { return mSaved; }
    const fs::path& path() { return mPath; }

    draw::shared::Vic20Screen& screen() { return mScreen; }
    uint8_t characterAt(size_t index) const {
        uint32_t offset = index / sizeof(draw::shared::ScreenElement);
        uint32_t byte = index % sizeof(draw::shared::ScreenElement);
        return (mScreen.screen[offset] >> (byte * 8)) & 0xFF;
    }

    void writeCharacter(size_t index, uint8_t character) {
        uint32_t offset = index / sizeof(draw::shared::ScreenElement);
        uint32_t byte = index % sizeof(draw::shared::ScreenElement);
        mScreen.screen[offset] &= ~(0xFF << (byte * 8));
        mScreen.screen[offset] |= (uint32_t(character) << (byte * 8));
        mSaved = false;
    }

    uint8_t colourAt(size_t index) const {
        uint32_t offset = index / sizeof(draw::shared::ScreenElement);
        uint32_t byte = index % sizeof(draw::shared::ScreenElement);
        return (mScreen.colour[offset] >> (byte * 8)) & 0xFF;
    }

    uint8_t bgColourAt(size_t index) const { return colourAt(index) & 0x0F; }
    uint8_t fgColourAt(size_t index) const { return (colourAt(index) >> 4) & 0x0F; }

    void writeColour(size_t index, uint8_t colour) {
        uint32_t offset = index / sizeof(draw::shared::ScreenElement);
        uint32_t byte = index % sizeof(draw::shared::ScreenElement);
        mScreen.colour[offset] &= ~(0xFF << (byte * 8));
        mScreen.colour[offset] |= (uint32_t(colour) << (byte * 8));
        mSaved = false;
    }

    const char *pathString() const { return mPathString.c_str(); }
    const char *nameString() const { return mName.c_str(); }
};

struct ScreenPixel {
    uint8_t colour;
    uint8_t index;
    draw::shared::Vic20Character character;
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

static const char *kPixelPayloadName = "TILE_DATA";

class CharacterMapWidget {
    void DrawSelectionGrid(size_t& selected) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImGui::BeginChild("ChildL", ImVec2(avail.x * 0.5f, avail.y));

        ImGuiStyle& style = ImGui::GetStyle();
        float2 size = { 40, 40 };
        float windowVisibleX2 = ImGui::GetCursorScreenPos().x + ImGui::GetContentRegionAvail().x;
        // draw grid
        for (size_t i = 0; i < charmap.characterCount(); i++) {
            ImGui::PushID(i);
            if (ImGui::Button(fmt::format("{}", i).c_str(), size)) {
                selected = i;
            }

            if (ImGui::BeginDragDropSource()) {
                ScreenPixel pixel = {
                    .colour = getColour(),
                    .index = uint8_t(i),
                    .character = charmap.characterAt(i),
                };
                ImGui::SetDragDropPayload(kPixelPayloadName, &pixel, sizeof(ScreenPixel));
                ImGui::EndDragDropSource();
            }

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(kPixelPayloadName)) {
                    ScreenPixel pixel = *reinterpret_cast<const ScreenPixel *>(payload->Data);
                    charmap.characterAt(i) = pixel.character;
                }
                ImGui::EndDragDropTarget();
            }

            float lastButtonX2 = ImGui::GetItemRectMax().x;
            float nextButtonX2 = lastButtonX2 + style.ItemSpacing.x + size.x;
            if (i + 1 < charmap.characterCount() && nextButtonX2 < windowVisibleX2) {
                ImGui::SameLine();
            }

            ImGui::PopID();
        }

        ImGui::EndChild();
    }

    void DrawEditorGrid(size_t& selected) {
        draw::shared::Vic20Character& character = charmap.characterAt(selected);

        ImGui::BeginChild("ChildR");
        std::string title = fmt::format("Selected: {} - {:0>64Lb}", selected, character.data);
        ImGui::TextUnformatted(title.c_str());

        PaletteColourPicker("Background", &bg);
        PaletteColourPicker("Foreground", &fg);

        if (ImGui::Button("Swap Colours")) {
            std::swap(fg, bg);
        }

        DrawCharacterEditor(character, fg, bg);

        ImGui::EndChild();
    }
public:
    CharacterMapDocument charmap;
    bool open = true;
    int fg = VIC20_COLOUR_WHITE;
    int bg = VIC20_COLOUR_BLACK;
    int first = 0;

    uint8_t getColour() const noexcept {
        return uint8_t(VIC20_CHAR_COLOUR(uint8_t(fg), uint8_t(bg)));
    }

    void Draw(size_t& selected, ScreenDocument *screen, Vic20DrawContext& context, std::list<CharacterMapWidget>& charmaps) {
        if (!open) return;

        auto title = fmt::format("Character Map - {}", charmap.nameString());
        ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
        if (!charmap.isSavedToDisk()) {
            flags |= ImGuiWindowFlags_UnsavedDocument;
        }

        if (ImGui::Begin(title.c_str(), &open, flags)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::MenuItem("Clone")) {
                    CharacterMapDocument doc = CharacterMapDocument::newInMemory(charmap);
                    charmaps.emplace_back(doc);
                }

                ImGui::EndMenuBar();
            }

            size_t size = charmap.characterCount();
            ImGui::Text("%s - %zu characters - %zu bytes", charmap.pathString(), size, charmap.size());

            ImGui::BeginDisabled(screen == nullptr);

            if (ImGui::Button("Draw Character Map")) {
                auto& data = screen->screen();

                for (size_t i = 0; i < size; i++) {
                    screen->writeCharacter(i, i);
                }

                memset(data.colour, VIC20_CHAR_COLOUR(fg, bg), sizeof(data.colour));

                context.setScreen(data);
            }

            ImGui::SameLine();

            if (ImGui::Button("Fill Screen")) {
                auto& data = screen->screen();
                memset(data.screen, uint8_t(selected), sizeof(data.screen));
                memset(data.colour, VIC20_CHAR_COLOUR(fg, bg), sizeof(data.colour));
                context.setScreen(data);
            }

            ImGui::SameLine();

            if (ImGui::DragInt("First Character", &first, 1.f, 0, size - UCHAR_MAX - 1)) {
                for (uint8_t i = 0; i > UCHAR_MAX; i++) {
                    context.writeCharacter(i, charmap.characters()[first + i]);
                }

                context.setScreen(screen->screen());
            }

            ImGui::EndDisabled();

            DrawSelectionGrid(selected);

            ImGui::SameLine();

            DrawEditorGrid(selected);
        }

        ImGui::End();
    }
};

class ScreenMemoryWidget {
    void DrawScreenGrid(size_t& selected, Vic20DrawContext& context) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(1, 1));

        int size = VIC20_SCREEN_CHARS_WIDTH * VIC20_SCREEN_CHARS_HEIGHT;
        for (int i = 0; i < size; i++) {
            ImGui::PushID(i);

            char label[32];
            if (showCharmapIndex) {
                (void)snprintf(label, sizeof(label), "%d##pixel", screen.characterAt(i));
            } else {
                (void)snprintf(label, sizeof(label), "%d##pixel", i);
            }

            if (ImGui::Button(label, ImVec2(30, 22))) {
                // do something
                selected = screen.characterAt(i);
            }

            ImGui::PopID();

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload(kPixelPayloadName, ImGuiDragDropFlags_AcceptBeforeDelivery)) {
                    ScreenPixel pixel = *reinterpret_cast<const ScreenPixel *>(payload->Data);
                    if (ImGui::IsKeyDown(ImGuiKey_LeftShift) || payload->IsDelivery()) {
                        screen.write(context, i, pixel.index, pixel.colour);
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::BeginDragDropSource()) {
                ScreenPixel pixel = {
                    .colour = screen.colourAt(i),
                    .index = uint8_t(screen.characterAt(i)),
                };
                ImGui::SetDragDropPayload(kPixelPayloadName, &pixel, sizeof(ScreenPixel));
                ImGui::EndDragDropSource();
            }

            if (i % VIC20_SCREEN_CHARS_WIDTH != VIC20_SCREEN_CHARS_WIDTH - 1) {
                ImGui::SameLine();
            }
        }

        ImGui::PopStyleVar();
    }
public:
    ScreenDocument screen;
    bool open = true;
    bool showCharmapIndex = false;
    bool showCharacter = false;
    size_t selected = 0;

    void Draw(CharacterMapDocument *charmap, Vic20DrawContext& context, std::list<ScreenMemoryWidget>& screens) {
        if (!open) return;

        ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar;
        if (!screen.isSavedToDisk()) {
            flags |= ImGuiWindowFlags_UnsavedDocument;
        }

        auto title = fmt::format("Screen Memory - {}", screen.nameString());
        if (ImGui::Begin(title.c_str(), &open, flags)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::MenuItem("Clone")) {
                    ScreenDocument doc = ScreenDocument::newInMemory(screen);
                    screens.emplace_back(doc);
                }

                ImGui::EndMenuBar();
            }

            ImGui::Checkbox("Show Charmap Index", &showCharmapIndex);
            ImGui::SameLine();

            ImGui::BeginDisabled(charmap == nullptr || charmap->characterCount() == 0);
            if (charmap == nullptr) {
                showCharacter = false;
            }
            ImGui::Checkbox("Show Character", &showCharacter);
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip))
                ImGui::SetTooltip("Load a character map ROM to enable this option");

            ImGui::EndDisabled();

            if (showCharacter) {
                ImVec2 avail = ImGui::GetContentRegionAvail();
                ImGui::BeginChild("ChildL", ImVec2(avail.x * 0.8f, avail.y));
                DrawScreenGrid(selected, context);
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("ChildR");
                DrawCharacterEditor(charmap->characterAt(selected), screen.fgColourAt(selected), screen.bgColourAt(selected));
                ImGui::EndChild();
            } else {
                DrawScreenGrid(selected, context);
            }

        }
        ImGui::End();
    }
};

static int commonMain(launch::LaunchResult& launch) noexcept try {
    ClientWindow clientWindow{"VIC20", &launch.getInfoDb()};
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

    fs::path home = sm::system::getProgramFolder();
    openCharacterMapRom.SetPwd(home);
    saveCharacterMapRom.SetPwd(home);
    openScreenMemoryMap.SetPwd(home);
    saveScreenMemoryMap.SetPwd(home);

    std::string error;
    bool showDemoWindow = false;

    std::list<ScreenMemoryWidget> screens;
    ScreenMemoryWidget *activeScreen = nullptr;

    ScreenDocument *pendingSaveScreen = nullptr;

    // the character map currently being displayed
    std::list<CharacterMapWidget> charmaps;
    CharacterMapWidget *activeCharMap = nullptr;

    CharacterMapDocument *pendingSaveCharMap = nullptr;
    size_t selected = 0;

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

                if (ImGui::MenuItem("New Character Map ROM")) {
                    CharacterMapWidget& widget = charmaps.emplace_back(CharacterMapDocument::newInMemory());
                    activeCharMap = &widget;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Open Screen Memory Map")) {
                    openScreenMemoryMap.Open();
                }

                if (ImGui::MenuItem("New Screen Memory Map")) {
                    ScreenMemoryWidget& widget = screens.emplace_back(ScreenDocument::newInMemory());
                    activeScreen = &widget;
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                    for (CharacterMapWidget& charmap : charmaps) {
                        if (charmap.charmap.hasBackingFile()) {
                            charmap.charmap.saveToDisk();
                        } else {
                            // TODO: if more than one charmap has no backing file this doesnt work
                            saveCharacterMapRom.Open();
                            pendingSaveCharMap = &charmap.charmap;
                        }
                    }

                    for (ScreenMemoryWidget& screen : screens) {
                        if (screen.screen.hasBackingFile()) {
                            screen.screen.saveToDisk();
                        } else {
                            saveScreenMemoryMap.Open();
                            pendingSaveScreen = &screen.screen;
                        }
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                if (!charmaps.empty()) {
                    ImGui::SeparatorText("Character Maps");
                    for (CharacterMapWidget& charmap : charmaps) {
                        ImGui::MenuItem(charmap.charmap.nameString(), nullptr, &charmap.open);
                    }
                }

                if (!screens.empty()) {
                    ImGui::SeparatorText("Screen Memory Maps");
                    for (ScreenMemoryWidget& screen : screens) {
                        ImGui::MenuItem(screen.screen.nameString(), nullptr, &screen.open);
                    }
                }

                ImGui::MenuItem("Demo Window", nullptr, &showDemoWindow);
                ImGui::EndMenu();
            }


            if (ImGui::BeginMenu("Edit", !screens.empty() || !charmaps.empty())) {
                for (ScreenMemoryWidget& screen : screens) {
                    bool isActive = activeScreen == &screen;
                    if (ImGui::MenuItem(screen.screen.nameString(), nullptr, &isActive)) {
                        activeScreen = &screen;
                    }
                }

                for (CharacterMapWidget& charmap : charmaps) {
                    bool isActive = activeCharMap == &charmap;
                    if (ImGui::MenuItem(charmap.charmap.nameString(), nullptr, &isActive)) {
                        activeCharMap = &charmap;
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        if (openCharacterMapRom.HasSelected()) {
            try {
                CharacterMapWidget& widget = charmaps.emplace_back(CharacterMapDocument::open(openCharacterMapRom.GetSelected()));
                activeCharMap = &widget;
                context.setCharacterMap(widget.charmap.characterMap());
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            openCharacterMapRom.ClearSelected();
        }

        if (saveCharacterMapRom.HasSelected()) {
            try {
                pendingSaveCharMap->saveToFile(saveCharacterMapRom.GetSelected());
                pendingSaveCharMap = nullptr;
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            saveCharacterMapRom.ClearSelected();
        }

        if (openScreenMemoryMap.HasSelected()) {
            try {
                ScreenMemoryWidget& widget = screens.emplace_back(ScreenDocument::open(openScreenMemoryMap.GetSelected()));
                activeScreen = &widget;
            } catch (const std::exception& e) {
                error = e.what();
                ImGui::OpenPopup("Error");
            }

            openScreenMemoryMap.ClearSelected();
        }

        if (saveScreenMemoryMap.HasSelected()) {
            try {
                pendingSaveScreen->saveToFile(saveScreenMemoryMap.GetSelected());
                pendingSaveScreen = nullptr;
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

        CharacterMapDocument *charmapDocument = activeCharMap ? &activeCharMap->charmap : nullptr;
        ScreenDocument *screenDocument = activeScreen ? &activeScreen->screen : nullptr;

        for (CharacterMapWidget& charmap : charmaps) {
            charmap.Draw(selected, screenDocument, context, charmaps);
        }

        for (ScreenMemoryWidget& screen : screens) {
            screen.Draw(charmapDocument, context, screens);
        }

        if (charmapDocument != nullptr) {
            context.setCharacterMap(charmapDocument->characterMap());
        }

        if (screenDocument != nullptr) {
            context.setScreen(screenDocument->screen());
        }

        if (ImGui::Begin("VIC20", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            D3D12_GPU_DESCRIPTOR_HANDLE srv = context.getVic20TargetSrv();

            // center the image
            ImGui::SetCursorPos(ImVec2((avail.x - kNtscSize.width) * 0.5f, (avail.y - kNtscSize.height) * 0.5f));
            ImGui::Image(std::bit_cast<ImTextureID>(srv), ImVec2(kNtscSize.width, kNtscSize.height));
        }
        ImGui::End();

        clientWindow.present();
    }

    return 0;
} catch (const std::exception& err) {
    LOG_ERROR(GlobalLog, "Unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "Unknown exception encountered.");
    return -1;
}

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "editor-logs.db" },
    .logPath = "editor.log",

    .infoDbConfig = { .host = "editor.db" },
};

SM_LAUNCH_MAIN("Editor", commonMain, kLaunchInfo)
