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

const int kPlayerRow = 21;
const int kGridLimit = 8;
const int kAlienRow = 2;

static void drawPlayerShip(draw::shared::Vic20Screen& screen, int player) {
    int offset = kPlayerRow * VIC20_SCREEN_CHARS_WIDTH + ENTITY_COLUMN(player);
    writeScreen(screen, offset, CC_PLAYER_SHIP_1, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
}

static void drawAlienShip(draw::shared::Vic20Screen& screen, int alien) {
    int offset = kAlienRow * VIC20_SCREEN_CHARS_WIDTH + ENTITY_COLUMN(alien);
    writeScreen(screen, offset, CC_BIG_SHIP, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
}

static void drawThunderGrid(draw::shared::Vic20Screen& screen) {
    for (int y = kAlienRow + 1; y < VIC20_SCREEN_CHARS_HEIGHT - kGridLimit; y += 2) {
        for (int x = 0; x < VIC20_SCREEN_CHARS_WIDTH; x++) {
            int i = y * VIC20_SCREEN_CHARS_WIDTH + x;
            if (x % 2 == 0) {
                writeScreen(screen, i, CC_FILLED_BOX, VIC20_CHAR_COLOUR(VIC20_COLOUR_DARK_PURPLE, VIC20_COLOUR_BLACK));
            }
        }
    }
}

static void drawCopyright(draw::shared::Vic20Screen& screen) {
    int row = (VIC20_SCREEN_CHARS_HEIGHT - 1) * VIC20_SCREEN_CHARS_WIDTH;
    writeScreen(screen, row + 0, CC_COPYRIGHT, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
    writeScreen(screen, row + 1, CC_BRAND0, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
    writeScreen(screen, row + 2, CC_BRAND1, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
    writeScreen(screen, row + 3, CC_BRAND2, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
    writeScreen(screen, row + 4, CC_BRAND3, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
    writeScreen(screen, row + 5, CC_BRAND4, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
    writeScreen(screen, row + 6, CC_BRAND5, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
    writeScreen(screen, row + 7, CC_BRAND6, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
}

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

    int alien = 0;
    bool aliendir = false;
    int alienspeed = 1;
    int shiprate = 5;
    int nextship = 0;

    float tickrate = 1.0f / 60.0f;
    float accumulator = 0.0f;

    float last = ImGui::GetTime();

    while (clientWindow.next()) {
        ImGui::DockSpaceOverViewport();

        if (ImGui::Begin("Game")) {
            ImGui::Text("Player: %d", player);
            ImGui::Text("Place: %d", player / VIC20_PPC);
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
        }

        player = std::clamp(player, 0, VIC20_SCREEN_WIDTH - 1);
        score = std::clamp(score, 0u, 99999u);
        highscore = std::max(score, highscore);

        draw::shared::Vic20Screen screen;
        memset(&screen, 0, sizeof(draw::shared::Vic20Screen));

        drawPlayerShip(screen, player);
        drawAlienShip(screen, alien);
        drawThunderGrid(screen);

        drawCopyright(screen);

        // for (int i = 0; i < VIC20_SCREEN_CHARBUFFER_SIZE; i++) {
        //     writeScreen(screen, i, i, VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK));
        // }

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
