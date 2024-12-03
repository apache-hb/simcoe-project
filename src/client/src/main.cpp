#include "stdafx.hpp"

#include "launch/launch.hpp"
#include "launch/appwindow.hpp"

#include "draw/next/vic20.hpp"

#include "system/storage.hpp"

#include <imgui/imgui.h>

#include "charmap.h"

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

static void writeCharacter(draw::shared::Vic20Screen& screen, size_t index, uint8_t character) {
    uint32_t offset = index / sizeof(draw::shared::ScreenElement);
    uint32_t byte = index % sizeof(draw::shared::ScreenElement);
    screen.screen[offset] &= ~(0xFF << (byte * 8));
    screen.screen[offset] |= (uint32_t(character) << (byte * 8));
}

static void writeColour(draw::shared::Vic20Screen& screen, size_t index, uint8_t colour) {
    uint32_t offset = index / sizeof(draw::shared::ScreenElement);
    uint32_t byte = index % sizeof(draw::shared::ScreenElement);
    screen.colour[offset] &= ~(0xFF << (byte * 8));
    screen.colour[offset] |= (uint32_t(colour) << (byte * 8));
}

static void writeScreen(draw::shared::Vic20Screen& screen, size_t index, uint8_t character, uint8_t colour) {
    writeCharacter(screen, index, character);
    writeColour(screen, index, colour);
}

#define VIC20_PPC (VIC20_SCREEN_WIDTH / VIC20_SCREEN_CHARS_WIDTH)

#define ENTITY_COLUMN(ent) ((ent) / VIC20_PPC)

#define PLAYER_COLOUR VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK)
#define ALIEN_COLOUR VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK)
#define CELL_COLOUR VIC20_CHAR_COLOUR(VIC20_COLOUR_DARK_PURPLE, VIC20_COLOUR_BLACK)

const int kPlayerRow = 20;
const int kGridLimit = 8;
const int kAlienRow = 2;
const int kPlayerSlide = 3;

struct GameState {
    static constexpr size_t kHeight = VIC20_SCREEN_CHARS_HEIGHT - kAlienRow - 1;
    static constexpr size_t kGridStart = VIC20_SCREEN_CHARS_HEIGHT - kGridLimit - 1;
    uint8_t cell[VIC20_SCREEN_CHARS_WIDTH * kHeight];

    uint8_t gridColours[VIC20_SCREEN_CHARS_WIDTH / 2];
};

static void drawPlayerShip(draw::shared::Vic20Screen& screen, int player, uint8_t colour) {
    player -= kPlayerSlide;
    int slide = player % VIC20_PPC;
    int offset = kPlayerRow * VIC20_SCREEN_CHARS_WIDTH + ENTITY_COLUMN(player);
    switch (slide) {
    case 0:
        writeScreen(screen, offset, CC_PLAYER_SHIP_0, colour);
        break;
    case 1:
        writeScreen(screen, offset, CC_PLAYER_SHIP_1, colour);
        break;
    case 2:
        writeScreen(screen, offset, CC_PLAYER_SHIP_2L, colour);
        writeScreen(screen, offset + 1, CC_PLAYER_SHIP_2R, colour);
        break;
    case 3:
        writeScreen(screen, offset, CC_PLAYER_SHIP_3L, colour);
        writeScreen(screen, offset + 1, CC_PLAYER_SHIP_3R, colour);
        break;
    case 4:
        writeScreen(screen, offset, CC_PLAYER_SHIP_4L, colour);
        writeScreen(screen, offset + 1, CC_PLAYER_SHIP_4R, colour);
        break;
    case 5:
        writeScreen(screen, offset, CC_PLAYER_SHIP_5L, colour);
        writeScreen(screen, offset + 1, CC_PLAYER_SHIP_5R, colour);
        break;
    case 6:
        writeScreen(screen, offset, CC_PLAYER_SHIP_6L, colour);
        writeScreen(screen, offset + 1, CC_PLAYER_SHIP_6R, colour);
        break;
    case 7:
        writeScreen(screen, offset, CC_PLAYER_SHIP_7L, colour);
        writeScreen(screen, offset + 1, CC_PLAYER_SHIP_7R, colour);
        break;

    default:
        break;
    }
}

static void drawAlienShip(draw::shared::Vic20Screen& screen, int alien, uint8_t colour) {
    int offset = kAlienRow * VIC20_SCREEN_CHARS_WIDTH + ENTITY_COLUMN(alien);
    writeScreen(screen, offset, CC_BIG_SHIP, colour);
}

static void drawThunderGrid(draw::shared::Vic20Screen& screen, GameState& state) {
    for (int y = kAlienRow + 1; y < VIC20_SCREEN_CHARS_HEIGHT - kGridLimit - 1; y += 2) {
        for (int x = 0; x < VIC20_SCREEN_CHARS_WIDTH; x++) {
            int i = y * VIC20_SCREEN_CHARS_WIDTH + x;
            if (x % 2 == 0) {
                writeScreen(screen, i, CC_FILLED_BOX, CELL_COLOUR);
            }
        }
    }

    for (int x = 0; x < VIC20_SCREEN_CHARS_WIDTH / 2; x++) {
        int y = VIC20_SCREEN_CHARS_HEIGHT - kGridLimit;
        int i = y * VIC20_SCREEN_CHARS_WIDTH + (x * 2);
        writeScreen(screen, i, CC_FILLED_BOX, state.gridColours[x]);
    }
}

static void drawGameState(draw::shared::Vic20Screen& screen, GameState& state) {
    for (size_t i = 0; i < std::size(state.cell); i++) {
        int x = i % VIC20_SCREEN_CHARS_WIDTH;
        int y = (i / VIC20_SCREEN_CHARS_WIDTH);

        if (state.cell[i] != 0) {
            writeScreen(screen, i, state.cell[i], VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
        }
    }
}

static void drawCopyright(draw::shared::Vic20Screen& screen) {
    uint8_t c = VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK);
    int row = (VIC20_SCREEN_CHARS_HEIGHT - 1) * VIC20_SCREEN_CHARS_WIDTH;
    writeScreen(screen, row + 0, CC_COPYRIGHT, c);
    writeScreen(screen, row + 1, CC_BRAND0, c);
    writeScreen(screen, row + 2, CC_BRAND1, c);
    writeScreen(screen, row + 3, CC_BRAND2, c);
    writeScreen(screen, row + 4, CC_BRAND3, c);
    writeScreen(screen, row + 5, CC_BRAND4, c);
    writeScreen(screen, row + 6, CC_BRAND5, c);
    writeScreen(screen, row + 7, CC_BRAND6, c);
}

static constexpr uint8_t kCellColours[] = {
    VIC20_COLOUR_LIGHT_YELLOW,
    VIC20_COLOUR_GREEN,
    VIC20_COLOUR_BLUE,
    VIC20_COLOUR_PINK,
    VIC20_COLOUR_LIGHT_RED,
};

static int commonMain(launch::LaunchResult& launch) noexcept try {
    ClientWindow clientWindow{"VIC20", &launch.getInfoDb()};
    Vic20DrawContext& context = clientWindow.getVic20Context();

    draw::shared::Vic20CharacterMap charmapUpload;
    memcpy(&charmapUpload, charmap, sizeof(draw::shared::Vic20CharacterMap));
    context.setCharacterMap(charmapUpload);

    int player = 0;
    int movespeed = 3;
    uint32_t score = 0;
    uint32_t highscore = 0;
    GameState state;

    memset(&state.cell, 0, sizeof(state.cell));
    memset(&state.gridColours, CELL_COLOUR, sizeof(state.gridColours));

    int alien = 0;
    bool aliendir = false;
    int alienspeed = 1;
    int shiprate = 5;
    int nextship = 0;

    float tickrate = 1.0f / 60.0f;
    float accumulator = 0.0f;
    int counter = 0;

    float last = ImGui::GetTime();

    while (clientWindow.next()) {
        ImGui::DockSpaceOverViewport();

        static bool debugchars = false;

        if (ImGui::Begin("Game")) {
            ImGui::Text("Player: %d", player);
            ImGui::Text("Place: %d", player / VIC20_PPC);
            ImGui::Checkbox("Debug Characters", &debugchars);
        }
        ImGui::End();

        float now = ImGui::GetTime();
        float delta = now - last;
        last = now;

        accumulator += delta;

        while (accumulator >= tickrate) {
            accumulator -= tickrate;
            score++;

            if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
                player -= movespeed;
            }

            if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
                player += movespeed;
            }

            if (aliendir) {
                alien += alienspeed;
            } else {
                alien -= alienspeed;
            }

            if (alien < 0 || alien >= VIC20_SCREEN_WIDTH - 1) {
                aliendir = !aliendir;
            }

            memset(&state.cell, 0, VIC20_SCREEN_CHARS_WIDTH);
            for (size_t i = 1; i < GameState::kHeight - 1; i++) {
                memcpy(&state.cell[i * VIC20_SCREEN_CHARS_WIDTH], &state.cell[(i + 1) * VIC20_SCREEN_CHARS_WIDTH], VIC20_SCREEN_CHARS_WIDTH);
            }
            memset(&state.cell[(GameState::kHeight - 1) * VIC20_SCREEN_CHARS_WIDTH], 0, VIC20_SCREEN_CHARS_WIDTH);

            for (int i = 0; i < VIC20_SCREEN_CHARS_WIDTH; i++) {
                if (i % 2 != 0) continue;

                int row = GameState::kGridStart * VIC20_SCREEN_CHARS_WIDTH;
                if (state.cell[row + i] != 0) {
                    state.cell[row + i] = 0;

                    state.gridColours[i / 2] = VIC20_CHAR_COLOUR(kCellColours[counter++ % std::size(kCellColours)], VIC20_COLOUR_BLACK);
                }
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            int shot = player % VIC20_PPC;
            int row = (GameState::kHeight - 1) * VIC20_SCREEN_CHARS_WIDTH;
            state.cell[row + ENTITY_COLUMN(player)] = CC_SHOT_0 + shot;
        }

        player = std::clamp(player, kPlayerSlide, VIC20_SCREEN_WIDTH - kPlayerSlide - 1);
        score = std::clamp(score, 0u, 99999u);
        highscore = std::max(score, highscore);

        draw::shared::Vic20Screen screen;
        memset(&screen, 0, sizeof(draw::shared::Vic20Screen));

        drawGameState(screen, state);
        drawPlayerShip(screen, player, PLAYER_COLOUR);
        drawAlienShip(screen, alien, ALIEN_COLOUR);
        drawThunderGrid(screen, state);

        drawCopyright(screen);

        if (debugchars) {
            for (int i = 0; i < VIC20_SCREEN_CHARBUFFER_SIZE; i++) {
                writeScreen(screen, i, i, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
            }
        }

        context.setScreen(screen);

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
    .logDbConfig = { .host = "client-logs.db" },
    .logPath = "client.log",

    .infoDbConfig = { .host = "client.db" },
};

SM_LAUNCH_MAIN("Client", commonMain, kLaunchInfo)
