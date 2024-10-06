#include "stdafx.hpp"

#include "config/config.hpp"
#include "config/parse.hpp"
#include "editor/panels/viewport.hpp"
#include "input/toggle.hpp"

#include "system/input.hpp"
#include "system/system.hpp"
#include "core/timer.hpp"

#include "editor/editor.hpp"

#include "config/config.hpp"

#include "editor/draw.hpp"
#include "render/render.hpp"

#include "world/ecs.hpp"
#include "game/ecs.hpp"

#include "db/connection.hpp"

#include "logs/structured/channels.hpp"

#include "archive/archive.hpp"
#include "launch/launch.hpp"

#include "archive.dao.hpp"

using namespace sm;
using namespace sm::math;
using namespace sm::math::literals;

namespace structured = sm::logs::structured;

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


class DefaultWindowEvents final : public system::IWindowEvents {
    db::Connection& mConnection;
    render::IDeviceContext *mContext = nullptr;
    system::DesktopInput *mInput = nullptr;

    static constexpr math::int2 kMinWindowSize = { 128, 128 };

    void saveWindowPlacement(const WINDOWPLACEMENT& placement) noexcept {
        if (db::DbError error = mConnection.tryInsert(archive::fromWindowPlacement(placement))) {
            LOG_WARN(GlobalLog, "update failed: {}", error);
        }
    }

    std::optional<WINDOWPLACEMENT> loadWindowPlacement() noexcept {
        auto result = mConnection.trySelectOne<sm::dao::archive::WindowPlacement>();
        if (!result.has_value()) {
            LOG_WARN(GlobalLog, "failed to load window placement: {}", result.error());
            return std::nullopt;
        }

        auto select = result.value();

        WINDOWPLACEMENT placement = archive::toWindowPlacement(select);

        if (LONG width = placement.rcNormalPosition.right - placement.rcNormalPosition.left; width < kMinWindowSize.x) {
            LOG_WARN(GlobalLog, "window placement width too small {}, ignoring possibly corrupted data", width);
            return std::nullopt;
        }

        if (LONG height = placement.rcNormalPosition.bottom - placement.rcNormalPosition.top; height < kMinWindowSize.y) {
            LOG_WARN(GlobalLog, "window placement height too small {}, ignoring possibly corrupted data", height);
            return std::nullopt;
        }

        return placement;
    }

    LRESULT event(system::Window &window, UINT message, WPARAM wparam, LPARAM lparam) override {
        if (mInput) mInput->window_event(message, wparam, lparam);
        return ImGui_ImplWin32_WndProcHandler(window.getHandle(), message, wparam, lparam);
    }

    void resize(system::Window &window, math::int2 size) override {
        if (size.x < kMinWindowSize.x || size.y < kMinWindowSize.y) {
            LOG_WARN(GlobalLog, "resize too small {}/{}, ignoring", size.x, size.y);
            return;
        }

        if (mContext != nullptr) {
            mContext->resize_swapchain(uint2(size));
        }

        saveWindowPlacement(window.getPlacement());
    }

    void create(system::Window &window) override {
        if (auto placement = loadWindowPlacement()) {
            LOG_INFO(GlobalLog, "create window with placement");
            window.setPlacement(*placement);
        } else {
            LOG_INFO(GlobalLog, "create window without placement");
            window.centerWindow(system::MultiMonitor::ePrimary);
        }
    }

    bool close(system::Window &window) override {
        saveWindowPlacement(window.getPlacement());
        return true;
    }

public:
    DefaultWindowEvents(db::Connection& connection)
        : mConnection(connection)
    {
        if (db::DbError error = connection.tryCreateTable(sm::dao::archive::WindowPlacement::getTableInfo())) {
            LOG_WARN(GlobalLog, "update failed: {}", error);
        }
    }

    void attachRenderContext(render::IDeviceContext *context) {
        mContext = context;
    }

    void attachInput(system::DesktopInput *input) {
        mInput = input;
    }
};

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

static void messageLoop(system::ShowWindow show) {
    system::WindowConfig window_config = {
        .mode = system::WindowMode::eWindowed,
        .width = 1280,
        .height = 720,
        .title = "Priority Zero",
    };

    db::Environment sqlite = db::Environment::create(db::DbType::eSqlite3);
    db::Connection connection = sqlite.connect({ .host = "editor.db" });

    DefaultWindowEvents events{connection};

    system::Window window{window_config, events};
    system::DesktopInput desktop_input{window};
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
            system::mouse::set_visible(!cameraActive.is_active());
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

static int editorMain(system::ShowWindow show) {
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

static int commonMain(system::ShowWindow show) noexcept try {
    LOG_INFO(GlobalLog, "SMC_DEBUG = {}", SMC_DEBUG);
    LOG_INFO(GlobalLog, "CTU_DEBUG = {}", CTU_DEBUG);

    int result = editorMain(show);

    return result;
} catch (const db::DbException& err) {
    LOG_ERROR(GlobalLog, "database error: {} ({})", err.what(), err.code());
    return -1;
} catch (const std::exception& err) {
    LOG_ERROR(GlobalLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception");
    return -1;
}

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "editor-logs.db" },
    .logPath = "editor.log",
};

int main(int argc, const char **argv) noexcept try {
    auto _ = launch::commonInit(GetModuleHandleA(nullptr), kLaunchInfo);

    sm::Span<const char*> args{argv, size_t(argc)};
    LOG_INFO(GlobalLog, "args = [{}]", fmt::join(args, ", "));

    if (int err = sm::parseCommandLine(argc, argv)) {
        return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
    }

    int result = commonMain(system::ShowWindow::eShow);

    LOG_INFO(GlobalLog, "editor exiting with {}", result);

    return result;
} catch (const db::DbException& err) {
    LOG_ERROR(GlobalLog, "database error: {}", err.error());
    return -1;
} catch (const std::exception& err) {
    LOG_ERROR(GlobalLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception");
    return -1;
}

int WINAPI WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) try {
    auto _ = launch::commonInit(hInstance, kLaunchInfo);

    LOG_INFO(GlobalLog, "lpCmdLine = {}", lpCmdLine);
    LOG_INFO(GlobalLog, "nShowCmd = {}", nShowCmd);
    // TODO: parse lpCmdLine

    int result = commonMain(system::ShowWindow{nShowCmd});

    LOG_INFO(GlobalLog, "editor exiting with {}", result);

    return result;
} catch (const db::DbException& err) {
    LOG_ERROR(GlobalLog, "database error: {}", err.error());
    return -1;
} catch (const std::exception& err) {
    LOG_ERROR(GlobalLog, "unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    LOG_ERROR(GlobalLog, "unknown unhandled exception");
    return -1;
}
