#include "stdafx.hpp"

#include "system/input.hpp"
#include "input/debounce.hpp"
#include "system/system.hpp"
#include "core/timer.hpp"

#include "threads/threads.hpp"

#include "archive/record.hpp"
#include "archive/io.hpp"

#include "editor/editor.hpp"

#include "config/config.hpp"

#include "editor/draw.hpp"
#include "render/render.hpp"

#include "game/game.hpp"

using namespace sm;
using namespace sm::math;
using sm::world::IndexOf;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

// TODO: clean up loggers

static std::string format_log(const logs::Message &message, const char *colour, const char *reset) {
    // we dont have fmt/chrono.h because we use it in header only mode
    // so pull out the hours/minutes/seconds/milliseconds manually

    auto hours = (message.timestamp / (60 * 60 * 1000)) % 24;
    auto minutes = (message.timestamp / (60 * 1000)) % 60;
    auto seconds = (message.timestamp / 1000) % 60;
    auto milliseconds = message.timestamp % 1000;

    auto header = fmt::format("{}[{}]{}[{:02}:{:02}:{:02}.{:03}] {}:", colour,
                   message.severity, reset, hours, minutes, seconds, milliseconds,
                   message.category.name());

    return header;
}

class FileLog final : public logs::ILogChannel {
    io_t *mFile;

    void accept(const logs::Message &message) override {
        auto header = format_log(message, "", "");

        // ranges is impossible to use without going through a bunch of hoops
        // just iterate over the message split by newlines

        auto it = message.message.begin();
        auto end = message.message.end();

        while (it != end) {
            auto next = std::find(it, end, '\n');
            auto line = std::string_view{&*it, int_cast<size_t>(std::distance(it, next))};
            it = next;

            io_write(mFile, header.data(), header.size());
            io_write(mFile, line.data(), line.size());
            io_write(mFile, "\n", 1);

            if (it != end) {
                ++it;
            }
        }
    }

public:
    constexpr FileLog(io_t *io)
        : mFile(io)
    { }
};

class ConsoleLog final : public logs::ILogChannel {
    static constexpr colour_t get_colour(logs::Severity severity) {
        using Reflect = ctu::TypeInfo<logs::Severity>;
        CTASSERTF(severity.is_valid(), "invalid severity: %s", Reflect::to_string(severity).data());

        using logs::Severity;
        switch (severity) {
        case Severity::eTrace: return eColourWhite;
        case Severity::eInfo: return eColourGreen;
        case Severity::eWarning: return eColourYellow;
        case Severity::eError: return eColourRed;
        case Severity::ePanic: return eColourMagenta;
        default: return eColourDefault;
        }
    }

    void accept(const logs::Message &message) override {
        const auto pallete = &kColourDefault;

        const char *colour = colour_get(pallete, get_colour(message.severity));
        const char *reset = colour_reset(pallete);

        auto header = format_log(message, colour, reset);

        // ranges is impossible to use without going through a bunch of hoops
        // just iterate over the message split by newlines

        auto it = message.message.begin();
        auto end = message.message.end();

        while (it != end) {
            auto next = std::find(it, end, '\n');
            auto line = std::string_view{&*it, int_cast<size_t>(std::distance(it, next))};
            it = next;

            fmt::println("{} {}", header, line);

            if (it != end) {
                ++it;
            }
        }
    }

public:
    ConsoleLog() = default;
};

static print_backtrace_t print_options_make(io_t *io) {
    print_backtrace_t print = {
        .options = {.arena = sm::global_arena(), .io = io, .pallete = &kColourDefault},
        .header = eHeadingGeneric,
        .zero_indexed_lines = false,
        .project_source_path = SMC_SOURCE_DIR,
    };

    return print;
}

class DefaultSystemError final : public ISystemError {
    bt_report_t *mReport = nullptr;

    void error_begin(OsError error) override {
        mReport = bt_report_new(sm::global_arena());
        io_t *io = io_stderr();
        io_printf(io, "System error detected: (%s)\n", error.to_string());
    }

    void error_frame(bt_address_t it) override {
        bt_report_add(mReport, it);
    }

    void error_end() override {
        const print_backtrace_t kPrintOptions = print_options_make(io_stderr());
        print_backtrace(kPrintOptions, mReport);
        std::exit(CT_EXIT_INTERNAL); // NOLINT
    }
};

class DefaultWindowEvents final : public sys::IWindowEvents {
    archive::RecordStore &mStore;

    sys::WindowPlacement *mWindowPlacement = nullptr;
    archive::RecordLookup mPlacementLookup;

    render::Context *mContext = nullptr;
    sys::DesktopInput *mInput = nullptr;

    LRESULT event(sys::Window &window, UINT message, WPARAM wparam, LPARAM lparam) override {
        if (mInput) mInput->window_event(message, wparam, lparam);
        return ImGui_ImplWin32_WndProcHandler(window.get_handle(), message, wparam, lparam);
    }

    void resize(sys::Window &, math::int2 size) override {
        if (mContext != nullptr) {
            mContext->resize_swapchain(size.as<uint>());
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

    void attach_render(render::Context *context) {
        mContext = context;
    }

    void attach_input(sys::DesktopInput *input) {
        mInput = input;
    }
};

constinit static DefaultSystemError gDefaultError{};
static constinit ConsoleLog gConsoleLog{};

static void common_init(void) {
    bt_init();
    os_init();

    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        io_t *io = io_stderr();
        arena_t *arena = global_arena();

        const print_backtrace_t kPrintOptions = print_options_make(io);

        auto message = sm::vformat(msg, args);

        logs::gGlobal.log(logs::Severity::ePanic, "{}", message);

        bt_report_t *report = bt_report_collect(arena);
        print_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };

    auto& logger = logs::get_logger();
    logger.add_channel(&gConsoleLog);
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

    fs::path bundle_path = sm::get_appdir() / "bundle.tar";
    IoHandle tar = io_file(bundle_path.string().c_str(), eOsAccessRead, sm::global_arena());
    sm::Bundle bundle{*tar, archive::BundleFormat::eTar};

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
            .size = client.as<uint>(),
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

    context.create();

    auto& world = context.mWorld;

    game::Context game = game::init(world, context.get_active_camera());

    ed::Editor editor{context};

    world::Cube floorShape = { .width = 15.f, .height = 1.f, .depth = 15.f };
    world::Sphere bodyShape = { .radius = 1.f, .slices = 8, .stacks = 8 };

    world::Cube wallShape = { .width = 1.f, .height = 2.f, .depth = 1.f };

    world::Cylinder playerShape = { .radius = 0.7f, .height = 1.3f, .slices = 8 };

    game::PhysicsBody floor = game.addPhysicsBody(floorShape, 0.f, quatf::identity());
    game::PhysicsBody body = game.addPhysicsBody(bodyShape, world::kVectorUp * 5.f, quatf::identity(), true);
    game::CharacterBody player = game.addCharacterBody(playerShape, float3(0.f, 5.f, 10.f), quatf::identity(), true);

    player.setUpVector(world::kVectorUp);

    IndexOf<world::Material> groundMaterial = world.add(world::Material {
        .name = "Ground Material",
        .albedo = float3(0.5f, 0.5f, 0.5f)
    });

    IndexOf<world::Material> wallMaterial = world.add(world::Material {
        .name = "Wall Material",
        .albedo = float3(0.5f, 0.5f, 0.5f)
    });

    IndexOf<world::Material> bodyMaterial = world.add(world::Material {
        .name = "Body Material",
        .albedo = float3(0.5f, 0.5f, 0.5f)
    });

    IndexOf<world::Model> floorModel;
    IndexOf<world::Model> bodyModel;
    IndexOf<world::Model> playerModel;
    IndexOf<world::Model> wallModel;

    context.upload([&] {
        floorModel = world.add(world::Model {
            .name = "Floor Model",
            .mesh = floorShape,
            .material = groundMaterial
        });

        bodyModel = world.add(world::Model {
            .name = "Body Model",
            .mesh = bodyShape,
            .material = bodyMaterial
        });

        playerModel = world.add(world::Model {
            .name = "Player Model",
            .mesh = playerShape,
            .material = bodyMaterial
        });

        wallModel = world.add(world::Model {
            .name = "Wall Model",
            .mesh = wallShape,
            .material = wallMaterial
        });

        context.upload_model(floorModel);
        context.upload_model(bodyModel);
        context.upload_model(playerModel);
        context.upload_model(wallModel);
    });

    IndexOf<world::Node> floorNode = world.addNode(world::Node {
        .parent = context.get_scene().root,
        .name = "Floor",
        .transform = {
            .position = 0.f,
            .rotation = quatf::identity(),
            .scale = 1.f
        },
        .models = { floorModel }
    });

    IndexOf<world::Node> bodyNode = world.addNode(world::Node {
        .parent = context.get_scene().root,
        .name = "Body",
        .transform = {
            .position = world::kVectorUp * 5.f,
            .rotation = quatf::identity(),
            .scale = 1.f
        },
        .models = { bodyModel }
    });

    IndexOf<world::Node> playerNode = world.addNode(world::Node {
        .parent = context.get_scene().root,
        .name = "Player",
        .transform = {
            .position = float3(0.f, 0.f, 4.f),
            .rotation = quatf::identity(),
            .scale = 1.f
        },
        .models = { playerModel }
    });

    std::string_view walls = ""
        "1111111111"
        "1010000001"
        "1011110001"
        "1000100001"
        "1110100011"
        "1000111001"
        "1000000001"
        "1111111111"
    ;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 10; j++) {
            if (walls[i * 10 + j] == '1') {
                float3 pos = float3((j - 5) * 2, (i - 4) * 2, 2.5f);
                IndexOf wall = world.addNode(world::Node {
                    .parent = context.get_scene().root,
                    .name = "Wall",
                    .transform = {
                        .position = pos,
                        .rotation = quatf::identity(),
                        .scale = 1.f
                    },
                    .models = { wallModel }
                });

                game::PhysicsBody wallBody = game.addPhysicsBody(wallShape, pos, quatf::identity(), false);
                editor.addPhysicsBody(wall, std::move(wallBody));
            }
        }
    }

#if 0
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (i == 0 || i == 9 || j == 0 || j == 9) {
                float3 pos = float3((i - 7) * 2, (j - 7) * 2, 2.5f);
                IndexOf wall = world.addNode(world::Node {
                    .parent = context.get_scene().root,
                    .name = "Wall",
                    .transform = {
                        .position = pos,
                        .rotation = quatf::identity(),
                        .scale = 1.f
                    },
                    .models = { wallModel }
                });

                game::PhysicsBody wallBody = game.addPhysicsBody(wallShape, pos, quatf::identity(), false);
                editor.addPhysicsBody(wall, std::move(wallBody));
            }
        }
    }

    for (int i = 0; i < 10; i++) {
        float3 pos = float3((i - 5) * 2, 5.f, (float(i * 6.f) / 10) - 2.f);
        IndexOf wall = world.addNode(world::Node {
            .parent = context.get_scene().root,
            .name = "Wall",
            .transform = {
                .position = pos,
                .rotation = quatf::identity(),
                .scale = 1.f
            },
            .models = { wallModel }
        });

        game::PhysicsBody wallBody = game.addPhysicsBody(wallShape, pos, quatf::identity(), false);
        editor.addPhysicsBody(wall, std::move(wallBody));
    }
#endif

    context.upload([&] {
        for (int i = 0; i < 16; i++) {
            float randx = (rand() % 16) - 8;
            float randy = (rand() % 16) - 8;
            float z = 6.f;

            float radius = 0.5f + (rand() % 10) / 10.f;

            float3 pos = float3(randx, randy, z);
            world::Sphere shape = { radius, 8, 8 };
            IndexOf sphere = world.add(world::Model {
                .name = "Sphere",
                .mesh = shape,
                .material = bodyMaterial
            });

            IndexOf it = world.addNode(world::Node {
                .parent = context.get_scene().root,
                .name = "Sphere",
                .transform = {
                    .position = pos,
                    .rotation = quatf::identity(),
                    .scale = 1.f
                },
                .models = { sphere }
            });

            game::PhysicsBody body = game.addPhysicsBody(shape, pos, quatf::identity(), true);
            body.activate();
            editor.addPhysicsBody(it, std::move(body));

            context.upload_model(sphere);
        }
    });

    Ticker clock;

    input::Debounce jump{input::Button::eSpace};

    {
        float3 p = float3(0.f, 23.f, 3.f);
        IndexOf groundNode = world.addNode(world::Node {
            .parent = context.get_scene().root,
            .name = "Ground",
            .transform = {
                .position = p,
                .rotation = quatf::identity(),
                .scale = 1.f
            },
            .models = { floorModel }
        });

        game::PhysicsBody ground = game.addPhysicsBody(floorShape, p, quatf::identity());
        editor.addPhysicsBody(groundNode, std::move(ground));
    }

    editor.addPhysicsBody(floorNode, std::move(floor));
    editor.addPhysicsBody(bodyNode, std::move(body));

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

        player.update(dt);

        game.tick(dt);
        context.tick(dt);

        player.postUpdate();
        editor.update();

        editor.begin_frame();

        world::Node& playerNodeInfo = world.get(playerNode);

        auto& state = context.input.get_state();
        static constexpr input::ButtonAxis kMoveForward = {input::Button::eW, input::Button::eS};
        static constexpr input::ButtonAxis kMoveStrafe =  {input::Button::eD, input::Button::eA};

        float2 move = state.button_axis2d(kMoveStrafe, kMoveForward);

        float3 moveInput = { move.x, move.y, 0.f };

        // rotate moveInput to face the direction the player is facing
        float3 forward = context.get_active_camera().direction();

        float3 right = float3::cross(forward, world::kVectorUp).normalized();

        moveInput = forward * -moveInput.y + right * -moveInput.x;

        if (player.isOnSteepSlope() || player.isNotSupported()) {
            float3 normal = player.getGroundNormal();
            normal.z = 0.f;
            float dot = float3::dot(normal, moveInput);
            if (dot < 0.f)
                moveInput -= (dot * normal) / normal.length_squared();
        }

        if (player.isSupported()) {
            float3 current = player.getLinearVelocity();
            float3 desired = 6.f * moveInput;
            desired.z = current.z;
            float3 newVelocity = 0.75f * current + 0.25f * desired;

            if (jump.is_pressed(state) && player.isOnGround())
                newVelocity += 8.f * world::kVectorUp;

            player.setLinearVelocity(newVelocity);
        }

        // player.setLinearVelocity(velocity);

        playerNodeInfo.transform.position = player.getPosition();
        playerNodeInfo.transform.rotation = player.getRotation();

        context.get_active_camera().setPosition(player.getPosition() + float3(0.f, 0.f, 2.f));

        // context.get_active_camera().set

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

    threads::CpuGeometry geometry = threads::global_cpu_geometry();

    threads::SchedulerConfig thread_config = {
        .worker_count = 8,
        .process_priority = threads::PriorityClass::eNormal,
    };
    threads::Scheduler scheduler{thread_config, geometry};

    if (!store.is_valid()) {
        store.reset();
    }

    init_imgui();
    message_loop(show, store);
    destroy_imgui();

    return 0;
}

static int common_main(sys::ShowWindow show) {
    logs::gGlobal.info("SMC_DEBUG = {}", SMC_DEBUG);
    logs::gGlobal.info("CTU_DEBUG = {}", CTU_DEBUG);

    int result = editor_main(show);

    logs::gGlobal.info("editor exiting with {}", result);

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

    System sys{GetModuleHandleA(nullptr)};

    if (!sm::parse_command_line(argc, argv, sys::get_appdir())) {
        return 0;
    }

    return common_main(sys::ShowWindow::eShow);
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    common_init();

    logs::gGlobal.info("lpCmdLine = {}", lpCmdLine);
    logs::gGlobal.info("nShowCmd = {}", nShowCmd);

    // TODO: parse lpCmdLine

    System sys{hInstance};

    return common_main(sys::ShowWindow{nShowCmd});
}
