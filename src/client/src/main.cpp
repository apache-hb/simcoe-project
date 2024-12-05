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

static uint8_t getCharacter(draw::shared::Vic20Screen& screen, size_t index) {
    uint32_t offset = index / sizeof(draw::shared::ScreenElement);
    uint32_t byte = index % sizeof(draw::shared::ScreenElement);
    return (screen.screen[offset] >> (byte * 8)) & 0xFF;
}

static uint8_t getColour(draw::shared::Vic20Screen& screen, size_t index) {
    uint32_t offset = index / sizeof(draw::shared::ScreenElement);
    uint32_t byte = index % sizeof(draw::shared::ScreenElement);
    return (screen.colour[offset] >> (byte * 8)) & 0xFF;
}

#define VIC20_PPC (VIC20_SCREEN_WIDTH / VIC20_SCREEN_CHARS_WIDTH)

#define ENTITY_COLUMN(ent) ((ent) / VIC20_PPC)

#define PLAYER_COLOUR VIC20_CHAR_COLOUR(VIC20_COLOUR_LIGHT_BLUE, VIC20_COLOUR_BLACK)
#define ALIEN_COLOUR VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK)
#define MISSILE_COLOUR VIC20_CHAR_COLOUR(VIC20_COLOUR_LIGHT_RED, VIC20_COLOUR_BLACK)
#define CELL_COLOUR VIC20_CHAR_COLOUR(VIC20_COLOUR_DARK_PURPLE, VIC20_COLOUR_BLACK)
#define SHOT_COLOUR VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK)

const int kPlayerRow = 20;
const int kGridLimit = 8;
const int kAlienRow = 3;
const int kPlayerSlide = 3;

static bool isPlayerShot(uint8_t c) {
    return c >= CC_SHOT_0 && c <= CC_SHOT_7;
}

enum { eEmpty, eMoveLeft, eMoveRight, eMoveDown };

struct Ship {
    uint8_t x;
    uint8_t y;
    int state = eEmpty;

    void update() {
        switch (state) {
        case eMoveLeft: x--; break;
        case eMoveRight: x++; break;
        case eMoveDown: y++; break;
        default: break;
        }
    }

    bool atLeftBoundary() const { return x <= 2; }
    bool atRightBoundary() const { return x >= VIC20_SCREEN_CHARS_WIDTH - 3; }

    void updateAction(int& rng) {
        if (state == eEmpty) {
            return;
        }

        int next = rng++;

        if (atLeftBoundary())
            next = 1 + (next % 2);
        else if (atRightBoundary())
            next = 1 - (next % 2);
        else
            next = next % 3;

        switch (next) {
        case 0: state = eMoveLeft; break;
        case 1: state = eMoveDown; break;
        case 2: state = eMoveRight; break;
        default: break;
        }
    }
};

struct Missile {
    uint8_t x;
    uint8_t y;
    int state = eEmpty;
};

struct GameState {
    static constexpr size_t kHeight = VIC20_SCREEN_CHARS_HEIGHT - kAlienRow - 1;
    static constexpr size_t kGridStart = VIC20_SCREEN_CHARS_HEIGHT - kGridLimit - 1;
    uint8_t cell[VIC20_SCREEN_CHARS_WIDTH * kHeight];

    uint8_t &getCell(int x, int y) {
        return cell[y * VIC20_SCREEN_CHARS_WIDTH + x];
    }

    Ship ships[8] = {};
    Missile missiles[16] = {};

    Ship *shipAt(int x, int y) {
        for (Ship& ship : ships) {
            if (ship.state != eEmpty && ship.x == x && ship.y == y) {
                return &ship;
            }
        }

        return nullptr;
    }

    Missile *missileAt(int x, int y) {
        for (Missile& missile : missiles) {
            if (missile.state != eEmpty && missile.x == x && missile.y == y) {
                return &missile;
            }
        }

        return nullptr;
    }

    void spawnShip(int alien) {
        int col = ENTITY_COLUMN(alien);
        if (col % 2 == 0) {
            return;
        }

        for (Ship& ship : ships) {
            if (ship.state == eEmpty) {
                ship.x = col;
                ship.y = 4;
                ship.state = eMoveDown;
                break;
            }
        }
    }

    uint8_t gridColours[VIC20_SCREEN_CHARS_WIDTH / 2];

    void movePlayerShots(uint32_t& score) {
        // clear all shots that go above the screen
        memset(&cell, 0, VIC20_SCREEN_CHARS_WIDTH * 2);

        for (size_t i = 1; i < GameState::kHeight - 1; i++) {
            for (int x = 0; x < VIC20_SCREEN_CHARS_WIDTH; x++) {

                // player shots move upwards
                uint8_t& v = getCell(x, i + 1);
                if (isPlayerShot(v)) {
                    if (Ship *ship = shipAt(x, i)) {
                        ship->state = eEmpty;
                        v = CC_ENEMY_HIT;
                        score += 25;
                    }
                    else if (Ship *ship = shipAt(x, i + 1)) {
                        ship->state = eEmpty;
                        v = CC_ENEMY_HIT;
                        score += 25;
                    }
                    else if (Missile *missile = missileAt(x, i)) {
                        missile->state = eEmpty;
                        v = CC_ENEMY_HIT;
                        score += 25;
                    }
                    else if (Missile *missile = missileAt(x, i + 1)) {
                        missile->state = eEmpty;
                        v = CC_ENEMY_HIT;
                        score += 25;
                    }

                    getCell(x, i) = v;
                    v = 0;
                }

                if (v == CC_ENEMY_HIT) {
                    v = 0;
                }
            }
        }
    }

    void addMissile(int x, int y, int dir) {
        for (Missile& missile : missiles) {
            if (missile.state == eEmpty) {
                missile.x = x;
                missile.y = y;
                missile.state = dir;
                break;
            }
        }
    }

    bool moveEnemyShips(int& rng, int player) {
        bool playerhit = false;
        for (Missile& missile : missiles) {
            if (missile.state == eEmpty) {
                continue;
            }

            if (missile.y == kHeight && missile.x == ENTITY_COLUMN(player)) {
                playerhit = true;
                missile.state = eEmpty;
                continue;
            }

            if (missile.y >= kHeight) {
                missile.state = eEmpty;
                continue;
            }


            if (missile.state == eMoveDown) {
                missile.y++;
                continue;
            }

            if (missile.state == eMoveLeft) {
                missile.x--;
                missile.y++;
                continue;
            }

            if (missile.state == eMoveRight) {
                missile.x++;
                missile.y++;
                continue;
            }
        }

        if (playerhit) {
            std::fill(std::begin(missiles), std::end(missiles), Missile{});
        }

        for (Ship& ship : ships) {
            if (ship.y >= kHeight) {
                ship.state = eEmpty;
                continue;
            }

            if (ship.state == eEmpty) {
                continue;
            }

            if (ship.y >= kGridStart) {
                ship.state = eEmpty;
                addMissile(ship.x, ship.y, eMoveDown);
                addMissile(ship.x, ship.y, eMoveLeft);
                addMissile(ship.x, ship.y, eMoveRight);
                continue;
            }

            if (ship.x % 2 != 0 && ship.y % 2 != 0) {
                ship.updateAction(rng);
            }

            ship.update();
        }

        return playerhit;
    }
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
    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < VIC20_SCREEN_CHARS_WIDTH / 2; x++) {
            int i = ((y * 2) + kAlienRow + 1) * VIC20_SCREEN_CHARS_WIDTH + (x * 2);
            writeScreen(screen, i, CC_FILLED_BOX, CELL_COLOUR);
        }
    }

    for (int x = 0; x < VIC20_SCREEN_CHARS_WIDTH / 2; x++) {
        int y = kAlienRow + 12 - 1;
        int i = y * VIC20_SCREEN_CHARS_WIDTH + (x * 2);
        writeScreen(screen, i, CC_FILLED_BOX, state.gridColours[x]);
    }
}

static void drawGameState(draw::shared::Vic20Screen& screen, GameState& state) {
    for (size_t i = 0; i < std::size(state.cell); i++) {
        int x = i % VIC20_SCREEN_CHARS_WIDTH;
        int y = (i / VIC20_SCREEN_CHARS_WIDTH);

        if (state.cell[i] != 0) {
            if (isPlayerShot(state.cell[i])) {
                writeScreen(screen, i, state.cell[i], SHOT_COLOUR);
            } else if (state.cell[i] == CC_ENEMY_HIT) {
                writeScreen(screen, i, CC_ENEMY_HIT, PLAYER_COLOUR);
            }
        }
    }

    for (Ship& ship : state.ships) {
        if (ship.state != eEmpty) {
            int offset = ship.y * VIC20_SCREEN_CHARS_WIDTH + ship.x;
            uint8_t tile = ship.y > (kAlienRow + 12) ? CC_ENEMY_MISSILE : CC_SMALL_SHIP;
            uint8_t colour = ship.y > (kAlienRow + 12) ? ALIEN_COLOUR : PLAYER_COLOUR;
            writeScreen(screen, offset, tile, colour);
        }
    }

    for (Missile& missile : state.missiles) {
        if (missile.state != eEmpty) {
            int offset = missile.y * VIC20_SCREEN_CHARS_WIDTH + missile.x;
            writeScreen(screen, offset, CC_ENEMY_BOMB, MISSILE_COLOUR);
        }
    }
}

static void drawTitle(draw::shared::Vic20Screen& screen) {
    uint8_t yellow = VIC20_CHAR_COLOUR(VIC20_COLOUR_LIGHT_YELLOW, VIC20_COLOUR_BLACK);
    uint8_t blue = VIC20_CHAR_COLOUR(VIC20_COLOUR_LIGHT_BLUE, VIC20_COLOUR_BLACK);
    uint8_t red = VIC20_CHAR_COLOUR(VIC20_COLOUR_LIGHT_RED, VIC20_COLOUR_BLACK);
    uint8_t green = VIC20_CHAR_COLOUR(VIC20_COLOUR_LIGHT_GREEN, VIC20_COLOUR_BLACK);
    uint8_t white = VIC20_CHAR_COLOUR(VIC20_COLOUR_WHITE, VIC20_COLOUR_BLACK);

    writeScreen(screen, 1, CC_TITLE_0, yellow); // t
    writeScreen(screen, 2, CC_TITLE_1, yellow); // h
    writeScreen(screen, 3, CC_TITLE_2, yellow); // u
    writeScreen(screen, 4, CC_TITLE_3, yellow); // n
    writeScreen(screen, 5, CC_TITLE_4, yellow); // d
    writeScreen(screen, 6, CC_TITLE_5, yellow); // e
    writeScreen(screen, 7, CC_TITLE_6, yellow); // r

    writeScreen(screen, 8, CC_TITLE_7, blue); // f
    writeScreen(screen, 9, CC_TITLE_8, blue); // l
    writeScreen(screen, 10, CC_TITLE_9, blue); // a
    writeScreen(screen, 11, CC_TITLE_10, blue); // s
    writeScreen(screen, 12, CC_TITLE_1, blue); // h

    writeScreen(screen, 13, CC_TITLE_NUM, red); // 2

    writeScreen(screen, 15, CC_BY, green);
    writeScreen(screen, 17, CC_AUTHOR_0, white);
    writeScreen(screen, 18, CC_AUTHOR_1, white);
    writeScreen(screen, 19, CC_AUTHOR_2, white);

    int row = (1 * VIC20_SCREEN_CHARS_WIDTH);
    writeScreen(screen, row + 17, CC_UNDERLINE_0, yellow);
    writeScreen(screen, row + 18, CC_UNDERLINE_1, yellow);
    writeScreen(screen, row + 19, CC_UNDERLINE_2, yellow);
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

static void drawScore(draw::shared::Vic20Screen& screen, uint32_t score) {
    int row = 2 * VIC20_SCREEN_CHARS_WIDTH;

    uint8_t sc = VIC20_CHAR_COLOUR(VIC20_COLOUR_PURPLE, VIC20_COLOUR_BLACK);

    writeScreen(screen, row + 2, CC_SCORE0, sc);
    writeScreen(screen, row + 3, CC_SCORE1, sc);
    writeScreen(screen, row + 4, CC_SCORE2, sc);

    char buffer[6];
    (void)snprintf(buffer, sizeof(buffer), "%05d", score);

    for (int i = 0; i < 5; i++) {
        writeScreen(screen, row + 5 + i, CC_NUMBER_0 + (buffer[i] - '0'), VIC20_CHAR_COLOUR(VIC20_COLOUR_BLACK, VIC20_COLOUR_LIGHT_YELLOW));
    }
}

static void drawHighScore(draw::shared::Vic20Screen& screen, uint32_t highscore) {
    int row = 2 * VIC20_SCREEN_CHARS_WIDTH;

    uint8_t sc = VIC20_CHAR_COLOUR(VIC20_COLOUR_LIGHT_YELLOW, VIC20_COLOUR_BLACK);

    int start = 11;

    writeScreen(screen, row + start + 0, CC_HIGHSCORE0, sc);
    writeScreen(screen, row + start + 1, CC_HIGHSCORE1, sc);
    writeScreen(screen, row + start + 2, CC_HIGHSCORE2, sc);

    char buffer[6];
    (void)snprintf(buffer, sizeof(buffer), "%05d", highscore);

    for (int i = 0; i < 5; i++) {
        writeScreen(screen, row + start + 3 + i, CC_NUMBER_0 + (buffer[i] - '0'), VIC20_CHAR_COLOUR(VIC20_COLOUR_BLACK, VIC20_COLOUR_LIGHT_YELLOW));
    }
}

static uint8_t nextCellColour(uint8_t current) {
    uint8_t fg = (current >> 4) & 0x0F;
    uint8_t bg = current & 0x0F;
    uint8_t next = (fg + 2) % 0x0F;
    if (next == VIC20_COLOUR_BLACK) {
        next = VIC20_COLOUR_WHITE;
    }

    return VIC20_CHAR_COLOUR(next, bg);
}

static const uint8_t kAlienColours[] = {
    VIC20_COLOUR_WHITE,
    VIC20_COLOUR_LIGHT_RED,
    VIC20_COLOUR_GREEN,
    VIC20_COLOUR_LIGHT_PURPLE,
    VIC20_COLOUR_BLUE,
    VIC20_COLOUR_LIGHT_BLUE,
};

void everyCountFrames(int count, auto&& fn) {
    static int counter = 0;
    if (counter % count == 0) {
        fn();
    }
    counter++;
}

static uint32_t gHighScore = 0;

struct MainGameLoop {

    int player = 0;
    int movespeed = 3;
    uint32_t score = 0;
    GameState state{};

    int alien = 0;
    bool aliendir = false;
    int alienspeed = 1;
    int shiprate = 16;
    int nextship = 0;
    int aliencolour = 0;

    float tickrate = 1.0f / 60.0f;
    float accumulator = 0.0f;
    int counter = 0;
    int rng = 0;
    int hitframe = -1;
    int health = 3;

    MainGameLoop() {
        memset(state.gridColours, VIC20_CHAR_COLOUR(VIC20_COLOUR_DARK_PURPLE, VIC20_COLOUR_BLACK), sizeof(state.gridColours));
    }

    bool update() {
        if (hitframe > -1) {
            hitframe -= 1;
            return true;
        }

        everyCountFrames(60, [&] {
            score += 10;
        });

        if (ImGui::IsKeyDown(ImGuiKey_LeftArrow) || ImGui::IsKeyDown(ImGuiKey_GamepadDpadLeft)) {
            player -= movespeed;
        }

        if (ImGui::IsKeyDown(ImGuiKey_RightArrow) || ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight)) {
            player += movespeed;
        }

        everyCountFrames(2, [&] {
            if (alien < 0 || alien >= VIC20_SCREEN_WIDTH - 1) {
                aliendir = !aliendir;
            }

            if (aliendir) {
                alien += alienspeed;
            } else {
                alien -= alienspeed;
            }
        });

        everyCountFrames(shiprate, [&] {
            state.spawnShip(alien);
        });

        // move all player shots up the screen

        everyCountFrames(2, [&] {
            state.movePlayerShots(score);
        });

        everyCountFrames(10, [&] {
            if (state.moveEnemyShips(rng, player)) {
                hitframe = 60;
                health -= 1;
            }
        });

        // if a player bullet hits a grid tile change its colour
        for (int i = 0; i < VIC20_SCREEN_CHARS_WIDTH; i++) {
            if (i % 2 != 0) continue;

            int row = GameState::kGridStart * VIC20_SCREEN_CHARS_WIDTH;
            if (state.cell[row + i] != 0) {
                state.cell[row + i] = 0;

                state.gridColours[i / 2] = nextCellColour(state.gridColours[i / 2]);
            }
        }

        everyCountFrames(10, [&] {
            aliencolour = (aliencolour + 1) % std::size(kAlienColours);
        });

        return health > 0 || hitframe <= -1;
    }

    void draw(draw::shared::Vic20Screen& screen) {
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceDown, false)) {
            int shot = player % VIC20_PPC;
            int row = (GameState::kHeight - 1) * VIC20_SCREEN_CHARS_WIDTH;
            state.cell[row + ENTITY_COLUMN(player)] = CC_SHOT_0 + shot;
        }

        player = std::clamp(player, kPlayerSlide, VIC20_SCREEN_WIDTH - kPlayerSlide - 1);
        score = std::clamp(score, 0u, 99999u);
        gHighScore = std::max(score, gHighScore);

        drawGameState(screen, state);
        drawPlayerShip(screen, player, PLAYER_COLOUR);
        drawAlienShip(screen, alien, VIC20_CHAR_COLOUR(kAlienColours[aliencolour], VIC20_COLOUR_BLACK));
        drawThunderGrid(screen, state);

        drawScore(screen, score);
        drawHighScore(screen, gHighScore);

        drawCopyright(screen);
        drawTitle(screen);

        if (hitframe > 0) {
            int rounded = hitframe / 15;

            // pick a random colour based on rounded and fill the background with it
            uint8_t colour = rounded % 15;
            for (int i = 0; i < VIC20_SCREEN_CHARS_WIDTH * VIC20_SCREEN_CHARS_HEIGHT; i++) {
                uint8_t colour = getColour(screen, i);
                colour = ((rounded + 7 % 15) << 4) | (rounded % 15);
                writeColour(screen, i, colour);
            }

            if (rounded % 2 == 0) {
                // slide all the characters to the right
                for (int i = 0; i < VIC20_SCREEN_CHARS_WIDTH * VIC20_SCREEN_CHARS_HEIGHT; i++) {
                    uint8_t c = getCharacter(screen, i);
                    if (c == 0) {
                        continue;
                    }

                    int x = i % VIC20_SCREEN_CHARS_WIDTH;
                    int y = i / VIC20_SCREEN_CHARS_WIDTH;

                    if (x == 0) {
                        continue;
                    }

                    writeScreen(screen, i - 1, c, getColour(screen, i - 1));
                    writeScreen(screen, i, 0, 0);
                }
            }
        }
    }
};

struct GameStartScreen {
    bool start = false;

    bool update() {
        if (ImGui::IsKeyDown(ImGuiKey_Space) || ImGui::IsKeyDown(ImGuiKey_GamepadFaceDown)) {
            start = true;
        }

        bool result = false;
        if (start) {
            result = true;
            start = false;
        }

        return result;
    }

    void draw(draw::shared::Vic20Screen& screen) {
        drawTitle(screen);
        drawScore(screen, gHighScore);
        drawHighScore(screen, gHighScore);
        drawCopyright(screen);

        int row = (6 * VIC20_SCREEN_CHARS_WIDTH);

        uint8_t c = VIC20_CHAR_COLOUR(VIC20_COLOUR_LIGHT_YELLOW, VIC20_COLOUR_BLACK);

        writeScreen(screen, row + 1, CC_P, c);
        writeScreen(screen, row + 2, CC_R, c);
        writeScreen(screen, row + 3, CC_E, c);
        writeScreen(screen, row + 4, CC_S, c);
        writeScreen(screen, row + 5, CC_S, c);

        writeScreen(screen, row + 7, CC_F, c);
        writeScreen(screen, row + 8, CC_I, c);
        writeScreen(screen, row + 9, CC_R, c);
        writeScreen(screen, row + 10, CC_E, c);

        writeScreen(screen, row + 12, CC_T, c);
        writeScreen(screen, row + 13, CC_O, c);

        writeScreen(screen, row + 15, CC_S, c);
        writeScreen(screen, row + 16, CC_T, c);
        writeScreen(screen, row + 17, CC_A, c);
        writeScreen(screen, row + 18, CC_R, c);
        writeScreen(screen, row + 19, CC_T, c);
    }
};

static int commonMain(launch::LaunchResult& launch) noexcept try {
    ClientWindow clientWindow{"VIC20", &launch.getInfoDb()};
    Vic20DrawContext& context = clientWindow.getVic20Context();

    draw::shared::Vic20CharacterMap charmapUpload;
    memcpy(&charmapUpload, charmap, sizeof(draw::shared::Vic20CharacterMap));
    context.setCharacterMap(charmapUpload);

    enum { eStart, eGame } state = eStart;

    MainGameLoop game;
    GameStartScreen start;

    float tickrate = 1.0f / 60.0f;
    float accumulator = 0.0f;

    float last = ImGui::GetTime();

    while (clientWindow.next()) {
        ImGui::DockSpaceOverViewport();

        float now = ImGui::GetTime();
        float delta = now - last;
        last = now;

        accumulator += delta;

        while (accumulator >= tickrate) {
            accumulator -= tickrate;

            if (state == eGame) {
                if (!game.update()) {
                    state = eStart;
                    game = MainGameLoop{};
                }
            } else {
                if (start.update()) {
                    state = eGame;
                }
            }
        }

        draw::shared::Vic20Screen screen;
        memset(&screen, 0, sizeof(draw::shared::Vic20Screen));

        if (state == eGame) {
            game.draw(screen);
        } else {
            start.draw(screen);
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
