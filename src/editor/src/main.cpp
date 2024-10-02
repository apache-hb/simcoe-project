#include "stdafx.hpp"

#include "config/config.hpp"
#include "config/parse.hpp"
#include "editor/panels/viewport.hpp"
#include "input/toggle.hpp"
#include "logs/logs.hpp"

#include "system/input.hpp"
#include "system/system.hpp"
#include "core/timer.hpp"

#include "logs/file.hpp"

#include "threads/threads.hpp"

#include "editor/editor.hpp"

#include "config/config.hpp"

#include "editor/draw.hpp"
#include "render/render.hpp"

#include "world/ecs.hpp"
#include "game/ecs.hpp"

#include "db/connection.hpp"

#include "core/defer.hpp"

#include "logs/structured/message.hpp"

#include "archive.dao.hpp"

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
    render::IDeviceContext *mContext = nullptr;
    sys::DesktopInput *mInput = nullptr;

    static constexpr math::int2 kMinWindowSize = { 128, 128 };

    void saveWindowPlacement(const WINDOWPLACEMENT& placement) noexcept {
        sm::dao::archive::WindowPlacement dao {
            .flags = placement.flags,
            .showCmd = placement.showCmd,
            .minPositionX = placement.ptMinPosition.x,
            .minPositionY = placement.ptMinPosition.y,
            .maxPositionX = placement.ptMaxPosition.x,
            .maxPositionY = placement.ptMaxPosition.y,
            .normalPosLeft = placement.rcNormalPosition.left,
            .normalPosTop = placement.rcNormalPosition.top,
            .normalPosRight = placement.rcNormalPosition.right,
            .normalPosBottom = placement.rcNormalPosition.bottom,
        };

        if (db::DbError error = mConnection.tryInsert(dao)) {
            logs::gAssets.warn("update failed: {}", error.message());
        }
    }

    std::optional<WINDOWPLACEMENT> loadWindowPlacement() noexcept {
        auto result = mConnection.trySelectOne<sm::dao::archive::WindowPlacement>();
        if (!result.has_value()) {
            logs::gGlobal.warn("failed to load window placement: {}", result.error().message());
            return std::nullopt;
        }

        auto select = result.value();

        WINDOWPLACEMENT placement = {
            .length = sizeof(WINDOWPLACEMENT),
            .flags = select.flags,
            .showCmd = select.showCmd,
            .ptMinPosition = {
                .x = (LONG)select.minPositionX,
                .y = (LONG)select.minPositionY,
            },
            .ptMaxPosition = {
                .x = (LONG)select.maxPositionX,
                .y = (LONG)select.maxPositionY,
            },
            .rcNormalPosition = {
                .left = (LONG)select.normalPosLeft,
                .top = (LONG)select.normalPosTop,
                .right = (LONG)select.normalPosRight,
                .bottom = (LONG)select.normalPosBottom,
            },
        };

        if (LONG width = placement.rcNormalPosition.right - placement.rcNormalPosition.left; width < kMinWindowSize.x) {
            logs::gGlobal.warn("window placement width too small {}, ignoring possibly corrupted data", width);
            return std::nullopt;
        }

        if (LONG height = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top; height < kMinWindowSize.y) {
            logs::gGlobal.warn("window placement height too small {}, ignoring possibly corrupted data", height);
            return std::nullopt;
        }

        return placement;
    }

    LRESULT event(sys::Window &window, UINT message, WPARAM wparam, LPARAM lparam) override {
        if (mInput) mInput->window_event(message, wparam, lparam);
        return ImGui_ImplWin32_WndProcHandler(window.getHandle(), message, wparam, lparam);
    }

    void resize(sys::Window &window, math::int2 size) override {
        if (size.x < kMinWindowSize.x || size.y < kMinWindowSize.y) {
            logs::gGlobal.warn("resize too small {}/{}, ignoring", size.x, size.y);
            return;
        }

        if (mContext != nullptr) {
            mContext->resize_swapchain(uint2(size));
        }

        saveWindowPlacement(window.getPlacement());
    }

    void create(sys::Window &window) override {
        if (auto placement = loadWindowPlacement()) {
            logs::gGlobal.info("create window with placement");
            window.setPlacement(*placement);
        } else {
            logs::gGlobal.info("create window without placement");
            window.centerWindow(sys::MultiMonitor::ePrimary);
        }
    }

    bool close(sys::Window &window) override {
        saveWindowPlacement(window.getPlacement());
        return true;
    }

public:
    DefaultWindowEvents(db::Connection& connection)
        : mConnection(connection)
    {
        if (db::DbError error = connection.tryCreateTable(sm::dao::archive::WindowPlacement::getTableInfo())) {
            logs::gAssets.warn("update failed: {}", error.message());
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

struct LogWrapper {
    db::Environment env;
    db::Connection connection;

    LogWrapper()
        : env(db::Environment::create(db::DbType::eSqlite3))
        , connection(env.connect({ .host = "editor-logs.db" }))
    {
        sm::logs::structured::setup(connection);
    }

    ~LogWrapper() {
        sm::logs::structured::cleanup();
    }
};

sm::UniquePtr<LogWrapper> gLogWrapper;

static void commonInit(void) {
    // bt_init();
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

    gLogWrapper = sm::makeUnique<LogWrapper>();

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

static void messageLoop(sys::ShowWindow show) {
    sys::WindowConfig window_config = {
        .mode = sys::WindowMode::eWindowed,
        .width = 1280,
        .height = 720,
        .title = "Priority Zero",
    };

    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    db::Connection connection = sqlite.connect({ .host = "editor.db" });

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
            .length = 8,
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

static int editorMain(sys::ShowWindow show) {
    ecs_os_set_api_defaults();
    ecs_os_api_t api = ecs_os_get_api();
    api.abort_ = [] {
        CT_NEVER("flecs.abort");
    };
    ecs_os_set_api(&api);

    init_imgui();
    messageLoop(show);
    destroy_imgui();

    return 0;
}

static int commonMain(sys::ShowWindow show) noexcept try {
    logs::gGlobal.info("SMC_DEBUG = {}", SMC_DEBUG);
    logs::gGlobal.info("CTU_DEBUG = {}", CTU_DEBUG);

    int result = editorMain(show);

    return result;
} catch (const db::DbException& err) {
    logs::gGlobal.error("database error: {} ({})", err.what(), err.code());
    return -1;
} catch (const std::exception& err) {
    logs::gGlobal.error("unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    logs::gGlobal.error("unknown unhandled exception");
    return -1;
}

int main(int argc, const char **argv) noexcept try {
    commonInit();
    defer { gLogWrapper.reset(); };

    sm::Span<const char*> args{argv, size_t(argc)};
    logs::gGlobal.info("args = [{}]", fmt::join(args, ", "));

    int result = [&] {
        sys::create(GetModuleHandleA(nullptr));
        defer { sys::destroy(); };

        if (int err = sm::parseCommandLine(argc, argv)) {
            return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
        }

        return commonMain(sys::ShowWindow::eShow);
    }();

    logs::gGlobal.info("editor exiting with {}", result);

    logs::shutdown();

    return result;
} catch (const db::DbException& err) {
    logs::gGlobal.error("database error: {} ({})", err.what(), err.code());
    return -1;
} catch (const std::exception& err) {
    logs::gGlobal.error("unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    logs::gGlobal.error("unknown unhandled exception");
    return -1;
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    commonInit();
    defer { gLogWrapper.reset(); };

    logs::gGlobal.info("lpCmdLine = {}", lpCmdLine);
    logs::gGlobal.info("nShowCmd = {}", nShowCmd);
    // TODO: parse lpCmdLine

    int result = [&] {
        sys::create(hInstance);
        defer { sys::destroy(); };

        return commonMain(sys::ShowWindow{nShowCmd});
    }();

    logs::gGlobal.info("editor exiting with {}", result);

    logs::shutdown();

    return result;
}
