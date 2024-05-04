#include "editor/panels/viewport.hpp"
#include "input/toggle.hpp"
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

using namespace sm;
using namespace sm::math;
using namespace sm::math::literals;

using sm::world::IndexOf;

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
        io_printf(io, "System error detected: (%s)\n", error.to_string().c_str());
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
            mContext->resize_swapchain(uint2(size));
        }
    }

    void create(sys::Window &window) override {
        logs::gGlobal.info("create window");
        if (mPlacementLookup = mStore.get_record(&mWindowPlacement); mPlacementLookup == archive::RecordLookup::eOpened) {
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

    void attach_render(render::IDeviceContext *context) {
        mContext = context;
    }

    void attach_input(sys::DesktopInput *input) {
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

        logs::gGlobal.panic("{}", message);

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
    events.attach_input(&desktop_input);

    window.show_window(show);

    auto client = window.get_client_coords().size();

    sm::Bundle bundle{sm::mountArchive(sm::get_appdir() / "bundle.tar")};

    render::DebugFlags flags = render::DebugFlags::none();

    if (sm::warp_enabled()) {
        flags |= render::DebugFlags::eWarpAdapter;
    }

    if (sm::debug_enabled()) {
        flags |= render::DebugFlags::eDeviceDebugLayer;
        flags |= render::DebugFlags::eFactoryDebug;
        flags |= render::DebugFlags::eInfoQueue;
        flags |= render::DebugFlags::eAutoName;
        flags |= render::DebugFlags::eDirectStorageDebug;

        // enabling gpu based validation on the warp adapter
        // tanks performance
        if (!sm::warp_enabled()) {
            flags |= render::DebugFlags::eGpuValidation;
        }
    }

    if (sm::pix_enabled()) {
        flags |= render::DebugFlags::eWinPixEventRuntime;
    }

    if (sm::dred_enabled()) {
        flags |= render::DebugFlags::eDeviceRemovedInfo;
    }

    const render::RenderConfig kRenderConfig = {
        .flags = flags,
        .preference = render::AdapterPreference::eMinimumPower,
        .feature_level = render::FeatureLevel::eLevel_11_0,

        .swapchain = {
            .size = uint2(client),
            .length = 2,
            .format = DXGI_FORMAT_R8G8B8A8_UNORM,
        },

        .rtv_heap_size = 64,
        .dsv_heap_size = 64,
        .srv_heap_size = 1024,

        .bundle = bundle,
        .window = window,
    };

    ed::EditorContext context{kRenderConfig};

    context.input.add_source(&desktop_input);

    events.attach_render(&context);

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

static int editor_main(sys::ShowWindow show) {
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

    if (!store.is_valid()) {
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

struct System {
    System(HINSTANCE hInstance) {
        sys::create(hInstance);
    }
    ~System() {
        sys::destroy();
    }
};

int main(int argc, const char **argv) {
    common_init();
    sm::Span<const char*> args{argv, size_t(argc)};
    logs::gGlobal.info("args = [{}]", fmt::join(args, ", "));

    int result = [&] {
        System sys{GetModuleHandleA(nullptr)};

        if (!sm::parse_command_line(argc, argv, sys::get_appdir())) {
            return 0;
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
        System sys{hInstance};

        return common_main(sys::ShowWindow{nShowCmd});
    }();

    logs::gGlobal.info("editor exiting with {}", result);

    logs::shutdown();

    return result;
}
