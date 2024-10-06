#include "stdafx.hpp"

#include "archive/archive.hpp"
#include "archive/io.hpp"

#include "draw/camera.hpp"

#include "system/input.hpp"
#include "system/system.hpp"
#include "core/timer.hpp"

#include "config/config.hpp"

#include "draw/draw.hpp"
#include "render/render.hpp"

#include "config/config.hpp"
#include "config/parse.hpp"

#include "db/connection.hpp"

#include "logs/structured/channels.hpp"

#include "launch/launch.hpp"

#include "archive.dao.hpp"

using namespace sm;
using namespace math;

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
            LOG_WARN(IoLog, "update failed: {}", error.message());
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
        return ImGui_ImplWin32_WndProcHandler(window.get_handle(), message, wparam, lparam);
    }

    void resize(system::Window &window, math::int2 size) override {
        if (size.x < kMinWindowSize.x || size.y < kMinWindowSize.y) {
            LOG_WARN(GlobalLog, "resize too small {}/{}, ignoring", size.x, size.y);
            return;
        }

        if (mContext != nullptr) {
            mContext->resize_swapchain(math::uint2(size));
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
            LOG_WARN(GlobalLog, "update failed: {}", error.message());
        }
    }

    void attachRenderContext(render::IDeviceContext *context) {
        mContext = context;
    }

    void attachInput(system::DesktopInput *input) {
        mInput = input;
    }
};

struct ClientContext final : public render::IDeviceContext {
    using Super = render::IDeviceContext;
    using Super::Super;

    draw::ViewportConfig viewport = {
        .size = { 1920, 1080 },
        .colour = DXGI_FORMAT_R8G8B8A8_UNORM,
        .depth = DXGI_FORMAT_D32_FLOAT,
    };

    graph::Handle mSceneTargetHandle;
    draw::Camera camera{"client", viewport};

    void setup_framegraph(graph::FrameGraph& graph) override {
        render::Viewport vp = render::Viewport::letterbox(getSwapChainSize(), viewport.size);

        graph::Handle depth;

        draw::opaque(graph, mSceneTargetHandle, depth, camera);
        draw::blit(graph, mSwapChainHandle, mSceneTargetHandle, vp);
    }
};

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
    db::Connection connection = sqlite.connect({ .host = "client.db" });

    DefaultWindowEvents events{connection};

    system::Window window{window_config, events};
    system::DesktopInput desktop_input{window};

    input::InputService input;
    events.attachInput(&desktop_input);
    input.add_source(&desktop_input);

    window.show_window(show);

    auto client = window.get_client_coords().size();

    // fs::path bundle_path = sys::getProgramFolder() / "bundle.tar";
    // IoHandle tar = io_file(bundle_path.string().c_str(), eOsAccessRead, get_default_arena());
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

    ClientContext context{renderConfig};

    events.attachRenderContext(&context);
    input.add_client(&context.camera);

    context.create();

    Ticker clock;

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
        input.poll();

        float dt = clock.tick();

        context.camera.tick(dt);

        context.render();
    }

    context.destroy();
}

static int clientMain(system::ShowWindow show) {
    messageLoop(show);
    return 0;
}

static int commonMain(system::ShowWindow show) {
    LOG_INFO(GlobalLog, "SMC_DEBUG = {}", SMC_DEBUG);
    LOG_INFO(GlobalLog, "CTU_DEBUG = {}", CTU_DEBUG);

    int result = clientMain(show);

    LOG_INFO(GlobalLog, "client exiting with {}", result);

    return result;
}

static const launch::LaunchInfo kLaunchInfo {
    .logDbType = db::DbType::eSqlite3,
    .logDbConfig = { .host = "server-logs.db" },
    .logPath = "server.log",
};

int main(int argc, const char **argv) noexcept try {
    auto _ = launch::commonInit(GetModuleHandleA(nullptr), kLaunchInfo);

    sm::Span<const char*> args{argv, size_t(argc)};
    LOG_INFO(GlobalLog, "args = [{}]", fmt::join(args, ", "));

    if (int err = sm::parseCommandLine(argc, argv)) {
        return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
    }

    return commonMain(system::ShowWindow::eShow);
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

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) try {
    auto _ = launch::commonInit(hInstance, kLaunchInfo);

    LOG_INFO(GlobalLog, "lpCmdLine = {}", lpCmdLine);
    LOG_INFO(GlobalLog, "nShowCmd = {}", nShowCmd);

    // TODO: parse lpCmdLine

    return commonMain(system::ShowWindow{nShowCmd});
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
