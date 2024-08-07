#include "config/option.hpp"
#include "editor/panels/viewport.hpp"
#include "input/toggle.hpp"
#include "orm/transaction.hpp"
#include "stdafx.hpp"

#include "system/input.hpp"
#include "system/system.hpp"
#include "core/timer.hpp"

#include "logs/file.hpp"

#include "threads/threads.hpp"

#include "archive/record.hpp"

#include "editor/editor.hpp"

#include "config/config.hpp"

#include "editor/draw.hpp"
#include "render/render.hpp"

#include "world/ecs.hpp"
#include "game/ecs.hpp"

#include "orm/connection.hpp"

#include "core/defer.hpp"

using namespace sm;
using namespace sm::math;
using namespace sm::math::literals;

using sm::world::IndexOf;

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

static fmt_backtrace_t print_options_make(io_t *io) {
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

class DefaultSystemError final : public ISystemError {
    bt_report_t *mReport = nullptr;

    void error_begin(OsError error) override {
        bt_update();

        mReport = bt_report_new(get_default_arena());
        io_t *io = io_stderr();
        io_printf(io, "System error detected: (%s)\n", error.toString().c_str());
    }

    void error_frame(bt_address_t it) override {
        bt_report_add(mReport, it);
    }

    void error_end() override {
        const fmt_backtrace_t kPrintOptions = print_options_make(io_stderr());
        fmt_backtrace(kPrintOptions, mReport);
        std::exit(CT_EXIT_INTERNAL); // NOLINT
    }
};

class DefaultWindowEvents final : public sys::IWindowEvents {
    db::Connection& mConnection;

    void saveWindowPlacement(const WINDOWPLACEMENT& placement) noexcept {
        auto result = mConnection.prepare(R"(
            INSERT INTO windowplacement (
                length, flags, show_cmd,
                min_position_x, min_position_y,
                max_position_x, max_position_y,
                normal_position_left, normal_position_top,
                normal_position_right, normal_position_bottom
            ) VALUES (
                :length, :flags, :show_cmd,
                :min_position_x, :min_position_y,
                :max_position_x, :max_position_y,
                :normal_position_left, :normal_position_top,
                :normal_position_right, :normal_position_bottom
            );
        )");

        if (!result) {
            logs::gGlobal.error("failed to prepare update: {}", result.error().message());
            return;
        }

        auto stmt = std::move(result.value());

        db::Transaction tx(&mConnection);

        stmt.bind("length") = (int64)placement.length;
        stmt.bind("flags") = (int64)placement.flags;
        stmt.bind("show_cmd") = (int64)placement.showCmd;
        stmt.bind("min_position_x") = (int64)placement.ptMinPosition.x;
        stmt.bind("min_position_y") = (int64)placement.ptMinPosition.y;
        stmt.bind("max_position_x") = (int64)placement.ptMaxPosition.x;
        stmt.bind("max_position_y") = (int64)placement.ptMaxPosition.y;
        stmt.bind("normal_position_left") = (int64)placement.rcNormalPosition.left;
        stmt.bind("normal_position_top") = (int64)placement.rcNormalPosition.top;
        stmt.bind("normal_position_right") = (int64)placement.rcNormalPosition.right;
        stmt.bind("normal_position_bottom") = (int64)placement.rcNormalPosition.bottom;

        if (auto update = stmt.update()) {
            logs::gGlobal.error("failed to execute update: {}", update.error().message());
            return;
        }
    }

    std::optional<WINDOWPLACEMENT> loadWindowPlacement() noexcept {
        auto result = mConnection.prepare(R"(
            SELECT
                length, flags, show_cmd,
                min_position_x, min_position_y,
                max_position_x, max_position_y,
                normal_position_left, normal_position_top,
                normal_position_right, normal_position_bottom
            FROM windowplacement;
        )");

        if (!result) {
            logs::gGlobal.error("failed to prepare query: {}", result.error().message());
            return std::nullopt;
        }

        auto stmt = std::move(result.value());

        auto select = TRY_RETURN(stmt.select(), (const auto& error) {
            logs::gGlobal.error("failed to execute select: {}", error.message());
            return std::nullopt;
        });

        if (db::DbError row = select.next()) {
            logs::gGlobal.error("failed to fetch row: {}", row.message());
            return std::nullopt;
        }

        WINDOWPLACEMENT placement = {};
        placement.length = (UINT)select.getInt("length");
        placement.flags = (UINT)select.getInt("flags");
        placement.showCmd = (UINT)select.getInt("show_cmd");
        placement.ptMinPosition.x = (LONG)select.getInt("min_position_x");
        placement.ptMinPosition.y = (LONG)select.getInt("min_position_y");
        placement.ptMaxPosition.x = (LONG)select.getInt("max_position_x");
        placement.ptMaxPosition.y = (LONG)select.getInt("max_position_y");
        placement.rcNormalPosition.left = (LONG)select.getInt("normal_position_left");
        placement.rcNormalPosition.top = (LONG)select.getInt("normal_position_top");
        placement.rcNormalPosition.right = (LONG)select.getInt("normal_position_right");
        placement.rcNormalPosition.bottom = (LONG)select.getInt("normal_position_bottom");

        return placement;
    }

    render::IDeviceContext *mContext = nullptr;
    sys::DesktopInput *mInput = nullptr;

    LRESULT event(sys::Window &window, UINT message, WPARAM wparam, LPARAM lparam) override {
        if (mInput) mInput->window_event(message, wparam, lparam);
        return ImGui_ImplWin32_WndProcHandler(window.get_handle(), message, wparam, lparam);
    }

    void resize(sys::Window &, math::int2 size) override {
        if (mContext != nullptr) {
            mContext->resize_swapchain(uint2(size));
        }
    }

    void create(sys::Window &window) override {
        logs::gGlobal.info("create window");
        if (auto placement = loadWindowPlacement()) {
            window.set_placement(*placement);
        } else {
            window.center_window(sys::MultiMonitor::ePrimary);
        }
    }

    bool close(sys::Window &window) override {
        saveWindowPlacement(window.get_placement());
        return true;
    }

public:
    DefaultWindowEvents(db::Connection& connection)
        : mConnection(connection)
    {
        if (!mConnection.tableExists("WINDOWPLACEMENT")) {
            auto result = mConnection.update(R"(
                CREATE TABLE WINDOWPLACEMENT (
                    length INTEGER NOT NULL,
                    flags INTEGER NOT NULL,
                    show_cmd INTEGER NOT NULL,
                    min_position_x INTEGER NOT NULL,
                    min_position_y INTEGER NOT NULL,
                    max_position_x INTEGER NOT NULL,
                    max_position_y INTEGER NOT NULL,
                    normal_position_left INTEGER NOT NULL,
                    normal_position_top INTEGER NOT NULL,
                    normal_position_right INTEGER NOT NULL,
                    normal_position_bottom INTEGER NOT NULL
                );
            )");

            if (!result.has_value()) {
                logs::gAssets.warn("failed to create WINDOWPLACEMENT table: {}", result.error().message());
            }
        }
    }

    void attachRenderContext(render::IDeviceContext *context) {
        mContext = context;
    }

    void attachInput(sys::DesktopInput *input) {
        mInput = input;
    }
};

static DefaultSystemError gDefaultError{};
static logs::FileChannel gFileChannel{};

static void common_init(void) {
    bt_init();
    os_init();

    // TODO: popup window for panics and system errors
    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        bt_update();
        io_t *io = io_stderr();
        arena_t *arena = get_default_arena();

        const fmt_backtrace_t kPrintOptions = print_options_make(io);

        auto message = sm::vformat(msg, args);

        fmt::println(stderr, "{}", message);

        bt_report_t *report = bt_report_collect(arena);
        fmt_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };

    auto& logger = logs::getGlobalLogger();

    if (logs::isConsoleHandleAvailable())
        logger.addChannel(logs::getConsoleHandle());

    if (logs::isDebugConsoleAvailable())
        logger.addChannel(logs::getDebugConsole());

    if (auto file = logs::FileChannel::open("editor.log"); file) {
        gFileChannel = std::move(file.value());
        logger.addChannel(gFileChannel);
    } else {
        logs::gGlobal.error("failed to open log file: {}", file.error());
    }

    threads::init();
}

static void init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImPlot::CreateContext();
    ImPlot::StyleColorsDark();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGuiStyle &style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

static void destroy_imgui() {
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}

static sm::IFileSystem *mountArchive(bool isPacked, const fs::path &path) {
    if (isPacked) {
        return sm::mountArchive(path);
    } else {
        return sm::mountFileSystem(path);
    }
}

static void message_loop(sys::ShowWindow show, archive::RecordStore &store) {
    sys::WindowConfig window_config = {
        .mode = sys::WindowMode::eWindowed,
        .width = 1280,
        .height = 720,
        .title = "Priority Zero",
    };

    db::Environment sqlite = TRY_RETURN(db::Environment::create(db::DbType::eSqlite3), (const auto& error) {
        logs::gGlobal.error("failed to create environment: {}", error.message());
    });

    db::Connection connection = TRY_RETURN(sqlite.connect({ .host = "editor.db" }), (const auto& error) {
        logs::gGlobal.error("failed to connect to database: {}", error.message());
    });

    DefaultWindowEvents events{connection};

    sys::Window window{window_config, events};
    sys::DesktopInput desktop_input{window};
    events.attachInput(&desktop_input);

    window.show_window(show);

    auto client = window.get_client_coords().size();

    sm::Bundle bundle{mountArchive(gBundlePacked.getValue(), gBundlePath.getValue())};

    const render::RenderConfig renderConfig = {
        .preference = render::AdapterPreference::eMinimumPower,
        .minFeatureLevel = render::FeatureLevel::eLevel_11_0,

        .swapchain = {
            .size = uint2(client),
            .length = 2,
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        },

        .rtvHeapSize = 64,
        .dsvHeapSize = 64,
        .srvHeapSize = 1024,

        .bundle = bundle,
        .window = window,
    };

    ed::EditorContext context{renderConfig};

    context.input.add_source(&desktop_input);

    events.attachRenderContext(&context);

    flecs::world& world = context.getWorld();
    world.import<flecs::monitor>();
    world.set<flecs::Rest>({});

    world.component<math::degf>()
        .member<float>("degrees");

    world.component<math::radf>()
        .member<float>("radians");

    world.component<math::float3>()
        .member<float>("x")
        .member<float>("y")
        .member<float>("z");

    world.component<math::quatf>()
        .member<math::float3>("v")
        .member<float>("angle");

    world.component<ed::ecs::MouseCaptured>()
        .member<bool>("captured");

    world::ecs::initSystems(world);
    game::ecs::initCameraSystems(world);

    context.create();

    flecs::entity floor = world.entity("Floor")
        .set<world::ecs::Position, world::ecs::Local>({ float3(0.f, 0.f, 0.f) })
        .set<world::ecs::Rotation>({ quatf::identity() })
        .set<world::ecs::Scale>({ 1.f })
        .add<world::ecs::Object>()
        .set<world::ecs::Shape>({ world::Cube{ 15.f, 1.f, 15.f } });

    world.entity("Player")
        .child_of(floor)
        .set<world::ecs::Position, world::ecs::Local>({ float3(0.f, 1.f, 0.f) })
        .set<world::ecs::Rotation>({ quatf::identity() })
        .set<world::ecs::Scale>({ 1.f })
        .add<world::ecs::Object>()
        .set<world::ecs::Shape>({ world::Cylinder{ 0.7f, 1.3f, 8 } });

    world.entity("Point Light")
        .set<world::ecs::Position, world::ecs::Local>({ float3(0.f, 5.f, 0.f) })
        .set<world::ecs::Colour>({ float3(1.f, 1.f, 1.f) })
        .set<world::ecs::Intensity>({ 1.f })
        .add<world::ecs::PointLight>();

    world.set<ed::ecs::MouseCaptured>({ false });

    ed::Editor editor{context};

    Ticker clock;

    input::Toggle cameraActive = false;

    bool done = false;
    while (!done) {
        MSG msg = {};
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT) {
                done = true;
            }
        }

        if (done) break;
        context.input.poll();

        float dt = clock.tick();

        const auto& state = context.input.getState();
        if (cameraActive.update(state.buttons[(size_t)input::Button::eTilde])) {
            context.input.capture_cursor(cameraActive.is_active());
            sys::mouse::set_visible(!cameraActive.is_active());
            world.set<ed::ecs::MouseCaptured>({ cameraActive.is_active() });
        }

        if (cameraActive.is_active()) {
            if (flecs::entity camera = ed::ecs::getPrimaryCamera(world)) {
                game::ecs::updateCamera(camera, dt, state);
            }
        }

        world.progress(dt);

        editor.begin_frame();

        editor.draw();

        editor.end_frame();

        context.render();
    }

    context.destroy();

    // TODO: this leaks but also actually freeing it causes a crash
    // game.shutdown();
}

static db::DbError sqliteTest() {
    auto env = TRY(db::Environment::create(db::DbType::eSqlite3), (const auto& error) {
        logs::gGlobal.error("failed to create environment: {}", error.message());
    });

    auto conn = TRY(env.connect({ .host = "test.db" }), (const auto& error) {
        logs::gGlobal.error("failed to connect to database: {}", error.message());
    });

    if (conn.tableExists("test")) {
        TRY(conn.update("DROP TABLE test"), (const auto& error) {
            logs::gGlobal.error("failed to drop table: {}", error.message());
        });
    }

    CTASSERT(!conn.tableExists("test"));

    TRY(conn.update("CREATE TABLE test (id INTEGER, name VARCHAR(100))"), (const auto& error) {
        logs::gGlobal.error("failed to create table: {}", error.message());
    });

    std::string_view sql = R"(
        CREATE TABLE IF NOT EXISTS test (
            id   INTEGER      NOT NULL PRIMARY KEY,
            name VARCHAR(255) NOT NULL
        ) STRICT;
    )";

    auto result = TRY(conn.prepare(sql), (const auto& error) {
        logs::gGlobal.error("failed to prepare query: {}", error.message());
    });

    TRY(result.update(), (const auto& error) {
        logs::gGlobal.error("failed to execute update: {}", error.message());
    });

    logs::gGlobal.info("executed query");

    return db::DbError::ok();
}

static db::DbError oradbTest() {
    auto env = TRY(db::Environment::create(db::DbType::eOracleDB), (const auto& error) {
        logs::gGlobal.error("failed to create environment: {}", error.message());
    });

    db::ConnectionConfig config = {
        .port = 1521,
        .host = "localhost",
        .user = "elliothb",
        .password = "elliothb",
        .database = "orclpdb"
    };

    auto conn = TRY(env.connect(config), (const auto& error) {
        logs::gGlobal.error("failed to connect to database: {}", error.message());
    });

    auto doUpdate = [&](std::string_view sql) {
        auto result = TRY(conn.prepare(sql), (const auto& error) {
            logs::gGlobal.error("failed to prepare query: {}", error.message());
        });

        TRY(result.update(), (const auto& error) {
            logs::gGlobal.error("failed to execute update: {}", error.message());
        });

        if (db::DbError err = conn.commit()) {
            logs::gGlobal.error("failed to commit transaction: {}", err.message());
        }

        logs::gGlobal.info("executed query");

        return db::DbError::ok();
    };

    doUpdate(R"(
        CREATE TABLE people (
            id   INTEGER      NOT NULL,
            name VARCHAR(255) NOT NULL,
            CONSTRAINT people_pk PRIMARY KEY (id)
        )
    )");

    doUpdate("INSERT INTO people VALUES (1, 'hello')");
    doUpdate("INSERT INTO people VALUES (2, 'world')");

    if (db::DbError err = conn.commit())
        logs::gGlobal.error("failed to commit transaction: {}", err.message());

    return db::DbError::ok();
}

static int editor_main(sys::ShowWindow show) {
    archive::RecordStoreConfig store_config = {
        .path = "client.bin",
        .size = {1, Memory::eMegabytes},
        .record_count = 256,
    };

    sqliteTest();
    oradbTest();

    archive::RecordStore store{store_config};

    const threads::CpuGeometry& geometry = threads::getCpuGeometry();

    threads::SchedulerConfig thread_config = {
        .workers = 8,
        .priority = threads::PriorityClass::eNormal,
    };
    threads::Scheduler scheduler{thread_config, geometry};

    if (!store.isValid()) {
        store.reset();
    }

    ecs_os_set_api_defaults();
    ecs_os_api_t api = ecs_os_get_api();
    api.abort_ = [] {
        CT_NEVER("flecs.abort");
    };
    ecs_os_set_api(&api);

    init_imgui();
    message_loop(show, store);
    destroy_imgui();

    return 0;
}

static int common_main(sys::ShowWindow show) {
    logs::gGlobal.info("SMC_DEBUG = {}", SMC_DEBUG);
    logs::gGlobal.info("CTU_DEBUG = {}", CTU_DEBUG);

    int result = editor_main(show);

    return result;
}

int main(int argc, const char **argv) {
    common_init();
    sm::Span<const char*> args{argv, size_t(argc)};
    logs::gGlobal.info("args = [{}]", fmt::join(args, ", "));

    int result = [&] {
        sys::create(GetModuleHandleA(nullptr));
        defer { sys::destroy(); };

        if (int err = sm::parseCommandLine(argc, argv)) {
            return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
        }

        return common_main(sys::ShowWindow::eShow);
    }();

    logs::gGlobal.info("editor exiting with {}", result);

    logs::shutdown();

    return result;
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    common_init();

    logs::gGlobal.info("lpCmdLine = {}", lpCmdLine);
    logs::gGlobal.info("nShowCmd = {}", nShowCmd);
    // TODO: parse lpCmdLine

    int result = [&] {
        sys::create(hInstance);
        defer { sys::destroy(); };

        return common_main(sys::ShowWindow{nShowCmd});
    }();

    logs::gGlobal.info("editor exiting with {}", result);

    logs::shutdown();

    return result;
}
