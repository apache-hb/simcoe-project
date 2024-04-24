#include "draw/draw.hpp"
#include "stdafx.hpp"

#include "system/input.hpp"
#include "input/debounce.hpp"
#include "system/system.hpp"
#include "core/timer.hpp"

#include "logs/file.hpp"

#include "threads/threads.hpp"

#include "archive/record.hpp"
#include "archive/io.hpp"

#include "editor/editor.hpp"

#include "config/config.hpp"

#include "editor/draw.hpp"
#include "render/render.hpp"

#include "game/game.hpp"

#include "world/ecs.hpp"

using namespace sm;
using namespace sm::math;
using namespace sm::math::literals;

using sm::world::IndexOf;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

static print_backtrace_t print_options_make(io_t *io) {
    print_backtrace_t print = {
        .options = {.arena = get_default_arena(), .io = io, .pallete = &kColourDefault},
        .header = eHeadingGeneric,
        .zero_indexed_lines = false,
        .project_source_path = SMC_SOURCE_DIR,
    };

    return print;
}

class DefaultSystemError final : public ISystemError {
    bt_report_t *mReport = nullptr;

    void error_begin(OsError error) override {
        mReport = bt_report_new(get_default_arena());
        io_t *io = io_stderr();
        io_printf(io, "System error detected: (%s)\n", error.to_string().c_str());
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

    render::IDeviceContext *mContext = nullptr;
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

    void attach_render(render::IDeviceContext *context) {
        mContext = context;
    }

    void attach_input(sys::DesktopInput *input) {
        mInput = input;
    }
};

constinit static DefaultSystemError gDefaultError{};
constinit static logs::FileChannel gFileChannel{};

static void common_init(void) {
    bt_init();
    os_init();

    // TODO: popup window for panics and system errors
    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        io_t *io = io_stderr();
        arena_t *arena = get_default_arena();

        const print_backtrace_t kPrintOptions = print_options_make(io);

        auto message = sm::vformat(msg, args);

        logs::gGlobal.panic("{}", message);

        bt_report_t *report = bt_report_collect(arena);
        print_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };

    auto& logger = logs::getGlobalLogger();

    if (logs::isConsoleHandleAvailable())
        logger.addChannel(logs::getConsoleHandle());

    if (logs::isDebugConsoleAvailable())
        logger.addChannel(logs::getDebugConsole());

    if (auto file = logs::FileChannel::open("editor.log"); file) {
        gFileChannel = std::move(*file);
        logger.addChannel(gFileChannel);
    }
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
    IoHandle tar = io_file(bundle_path.string().c_str(), eOsAccessRead, get_default_arena());
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

    flecs::world ecs;

    draw::init_ecs(context, ecs);

    flecs::entity camera = ecs.entity("camera")
        .set<world::ecs::Position>({ float3(0.f, 5.f, 10.f) })
        .set<world::ecs::Direction>({ float3(0.f, 0.f, 1.f) })
        .set<world::ecs::Camera>({
            .colour = DXGI_FORMAT_R8G8B8A8_UNORM,
            .depth = DXGI_FORMAT_D32_FLOAT,
            .window = client.as<uint>(),
            .fov = 90._deg
        });

    ecs.entity("player")
        .set<world::ecs::Position>({ float3(0.f, 3.f, 0.f) })
        .set<world::ecs::Rotation>({ quatf::identity() })
        .set<world::ecs::Scale>({ 1.f })
        .add<world::ecs::Object>()
        .set<world::ecs::Shape>({ world::Cylinder{ 0.7f, 1.3f, 8 } });

    ecs.entity("floor")
        .set<world::ecs::Position>({ float3(0.f, 0.f, 0.f) })
        .set<world::ecs::Rotation>({ quatf::identity() })
        .set<world::ecs::Scale>({ 1.f })
        .add<world::ecs::Object>()
        .set<world::ecs::Shape>({ world::Cube{ 15.f, 1.f, 15.f } });

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
            .position = float3(0.f, 4.f, 0.f),
            .rotation = quatf::identity(),
            .scale = 1.f
        },
        .models = { playerModel }
    });

#if 0
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
                float3 pos = float3((j - 5) * 2, 2.5f, (i - 4) * 2);
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

    context.upload([&] {
        for (int i = 0; i < 32; i++) {
            float randx = (rand() % 16) - 8;
            float randy = (rand() % 16) - 8;
            float height = 6.f;

            float radius = 0.5f + (rand() % 10) / 10.f;

            float3 pos = float3(randx, height, randy);
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
#endif

    Ticker clock;

    input::Debounce jump{input::Button::eSpace};

    {
        float3 p = float3(0.f, 3.f, 23.f);
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

        ecs.progress(dt);

        player.postUpdate();
        editor.update();

        editor.begin_frame();

        world::Node& playerNodeInfo = world.get(playerNode);

        auto& state = context.input.get_state();
        static constexpr input::ButtonAxis kMoveForward = {input::Button::eW, input::Button::eS};
        static constexpr input::ButtonAxis kMoveStrafe =  {input::Button::eD, input::Button::eA};

        float2 move = state.button_axis2d(kMoveStrafe, kMoveForward);

        // rotate moveInput to face the direction the player is facing
        float3 direction = context.get_active_camera().direction();

        float3 right = float3::cross(direction, world::kVectorUp).normalized();

        float3 moveInput = direction * -move.y + right * -move.x;

        if (player.isOnSteepSlope() || player.isNotSupported()) {
            float3 normal = player.getGroundNormal();
            normal.y = 0.f;
            float dot = float3::dot(normal, moveInput);
            if (dot < 0.f)
                moveInput -= (dot * normal) / normal.length_squared();
        }

        if (player.isSupported()) {
            float3 current = player.getLinearVelocity();
            float3 desired = 6.f * moveInput;
            desired.y = current.y;
            float3 newVelocity = 0.75f * current + 0.25f * desired;

            if (jump.is_pressed(state) && player.isOnGround())
                newVelocity += 8.f * world::kVectorUp;

            player.setLinearVelocity(newVelocity);
        }

        // player.setLinearVelocity(velocity);

        playerNodeInfo.transform.position = player.getPosition();
        playerNodeInfo.transform.rotation = player.getRotation();

        context.get_active_camera().setPosition(player.getPosition() + world::kVectorUp * 2);

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
