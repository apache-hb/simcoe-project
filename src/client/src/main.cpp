#include "stdafx.hpp"

#include "draw/camera.hpp"
#include "logs/file.hpp"

#include "system/input.hpp"
#include "system/system.hpp"
#include "core/timer.hpp"

#include "threads/threads.hpp"

#include "archive/record.hpp"

#include "config/config.hpp"

#include "draw/draw.hpp"
#include "render/render.hpp"

#include "config/config.hpp"
#include "config/parse.hpp"

#include "core/defer.hpp"

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
    archive::RecordStore &mStore;

    sys::WindowPlacement *mWindowPlacement = nullptr;
    archive::RecordLookup mPlacementLookup;

    render::IDeviceContext *mContext = nullptr;
    sys::DesktopInput *mInput = nullptr;

    LRESULT event(sys::Window &window, UINT message, WPARAM wparam, LPARAM lparam) override {
        if (mInput) mInput->window_event(message, wparam, lparam);
        return ImGui_ImplWin32_WndProcHandler(window.get_handle(), message, wparam, lparam);
    }

    void resize(sys::Window &, math::int2 size) override {
        if (mContext != nullptr) {
            mContext->resize_swapchain(math::uint2(size));
        }
    }

    void create(sys::Window &window) override {
        logs::gGlobal.info("create window");
        if (mPlacementLookup = mStore.getRecord(&mWindowPlacement); mPlacementLookup == archive::RecordLookup::eOpened) {
            window.set_placement(*mWindowPlacement);
        } else {
            window.center_window(sys::MultiMonitor::ePrimary);
        }
    }

    bool close(sys::Window &window) override {
        if (mPlacementLookup.has_valid_data()) *mWindowPlacement = window.get_placement();
        return true;
    }

public:
    DefaultWindowEvents(archive::RecordStore &store)
        : mStore(store)
    { }

    void attachRenderContext(render::IDeviceContext *context) {
        mContext = context;
    }

    void attachInput(sys::DesktopInput *input) {
        mInput = input;
    }
};

constinit static DefaultSystemError gDefaultError{};
static logs::FileChannel gFileChannel{};

static void commonInit(void) {
    bt_init();
    os_init();

    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        io_t *io = io_stderr();
        arena_t *arena = get_default_arena();

        const fmt_backtrace_t kPrintOptions = print_options_make(io);

        auto message = sm::vformat(msg, args);

        logs::gGlobal.log(logs::Severity::ePanic, "{}", message);

        bt_report_t *report = bt_report_collect(arena);
        fmt_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };

    auto& logger = logs::getGlobalLogger();

    if (logs::isConsoleHandleAvailable())
        logger.addChannel(logs::getConsoleHandle());

    if (logs::isDebugConsoleAvailable())
        logger.addChannel(logs::getDebugConsole());

    if (auto file = logs::FileChannel::open("client.log"); file) {
        gFileChannel = std::move(file.value());
        logger.addChannel(gFileChannel);
    } else {
        logs::gGlobal.error("failed to open log file: {}", file.error());
    }

    threads::init();
}

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

static void message_loop(sys::ShowWindow show, archive::RecordStore &store) {
    sys::WindowConfig window_config = {
        .mode = sys::WindowMode::eWindowed,
        .width = 1280,
        .height = 720,
        .title = "Priority Zero",
    };

    DefaultWindowEvents events{store};

    sys::Window window{window_config, events};
    sys::DesktopInput desktop_input{window};

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

static int client_main(sys::ShowWindow show) {
    archive::RecordStoreConfig store_config = {
        .path = "client.bin",
        .size = {1, Memory::eMegabytes},
        .record_count = 256,
    };

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

    message_loop(show, store);

    return 0;
}

static int commonMain(sys::ShowWindow show) {
    logs::gGlobal.info("SMC_DEBUG = {}", SMC_DEBUG);
    logs::gGlobal.info("CTU_DEBUG = {}", CTU_DEBUG);

    int result = client_main(show);

    logs::gGlobal.info("client exiting with {}", result);

    return result;
}

int main(int argc, const char **argv) {
    commonInit();

    sm::Span<const char*> args{argv, size_t(argc)};
    logs::gGlobal.info("args = [{}]", fmt::join(args, ", "));

    sys::create(GetModuleHandleA(nullptr));
    defer { sys::destroy(); };

    if (int err = sm::parseCommandLine(argc, argv)) {
        return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
    }

    return commonMain(sys::ShowWindow::eShow);
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    commonInit();

    logs::gGlobal.info("lpCmdLine = {}", lpCmdLine);
    logs::gGlobal.info("nShowCmd = {}", nShowCmd);

    // TODO: parse lpCmdLine

    sys::create(GetModuleHandleA(nullptr));
    defer { sys::destroy(); };

    return commonMain(sys::ShowWindow{nShowCmd});
}
