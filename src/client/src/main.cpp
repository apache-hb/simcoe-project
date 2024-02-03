#include "core/arena.hpp"
#include "core/backtrace.hpp"
#include "core/format.hpp"
#include "core/text.hpp"

#include "core/units.hpp"
#include "io/io.h"
#include "logs/logs.hpp"
#include "rhi/rhi.hpp"
#include "service/freetype.hpp"
#include "std/str.h"
#include "system/input.hpp"
#include "system/io.hpp"
#include "system/system.hpp"
#include "threads/threads.hpp"

#include "base/panic.h"
#include "ui/control.hpp"
#include "ui/render.hpp"

#include "backtrace/backtrace.h"
#include "format/backtrace.h"
#include "format/colour.h"
#include "io/console.h"
#include "os/os.h"

#include "imgui/backends/imgui_impl_dx12.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/imgui.h"
#include "imgui/misc/imgui_freetype.h"

#include "render/graph.hpp"
#include "render/render.hpp"
#include <array>

using namespace sm;

using GlobalSink = logs::Sink<logs::Category::eGlobal>;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

template <ctu::Reflected T>
static constexpr auto enum_to_string(T value) {
    using Reflect = ctu::TypeInfo<T>;
    return Reflect::to_string(value);
}

// TODO: clean up loggers

class FileLog final : public logs::ILogger {
    sm::FormatBuffer m_buffer;
    io_t *io;

    void accept(const logs::Message &message) override {
        const auto c = enum_to_string(message.category);
        const auto s = enum_to_string(message.severity);

        // we dont have fmt/chrono.h because we use it in header only mode
        // so pull out the hours/minutes/seconds/milliseconds manually

        auto hours = (message.timestamp / (60 * 60 * 1000)) % 24;
        auto minutes = (message.timestamp / (60 * 1000)) % 60;
        auto seconds = (message.timestamp / 1000) % 60;
        auto milliseconds = message.timestamp % 1000;

        fmt::format_to(std::back_inserter(m_buffer), "[{}][{:02}:{:02}:{:02}.{:03}] {}:", s.data(),
                       hours, minutes, seconds, milliseconds, c.data());

        std::string_view header{m_buffer.data(), m_buffer.size()};

        // ranges is impossible to use without going through a bunch of hoops
        // just iterate over the message split by newlines

        auto it = message.message.begin();
        auto end = message.message.end();

        while (it != end) {
            auto next = std::find(it, end, '\n');
            auto line = std::string_view{&*it, static_cast<size_t>(std::distance(it, next))};
            it = next;

            io_write(io, header.data(), header.size());
            io_write(io, line.data(), line.size());
            io_write(io, "\n", 1);

            if (it != end) {
                ++it;
            }
        }

        m_buffer.clear();
    }

public:
    constexpr FileLog(IArena &arena, io_t *io)
        : ILogger(logs::Severity::eInfo)
        , m_buffer(arena)
        , io(io) {}
};

class ConsoleLog final : public logs::ILogger {
    sm::FormatBuffer m_buffer;

    static constexpr colour_t get_colour(logs::Severity severity) {
        CTASSERTF(severity.is_valid(), "invalid severity: %s", enum_to_string(severity).data());

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
        const auto c = enum_to_string(message.category);
        const auto s = enum_to_string(message.severity);

        const auto pallete = &kColourDefault;

        const char *colour = colour_get(pallete, get_colour(message.severity));
        const char *reset = colour_reset(pallete);

        // we dont have fmt/chrono.h because we use it in header only mode
        // so pull out the hours/minutes/seconds/milliseconds manually

        auto hours = (message.timestamp / (60 * 60 * 1000)) % 24;
        auto minutes = (message.timestamp / (60 * 1000)) % 60;
        auto seconds = (message.timestamp / 1000) % 60;
        auto milliseconds = message.timestamp % 1000;

        fmt::format_to(std::back_inserter(m_buffer),
                       "{}[{}]{}[{:02}:{:02}:{:02}.{:03}] {}:", colour, s.data(), reset, hours,
                       minutes, seconds, milliseconds, c.data());

        std::string_view header{m_buffer.data(), m_buffer.size()};

        // ranges is impossible to use without going through a bunch of hoops
        // just iterate over the message split by newlines

        auto it = message.message.begin();
        auto end = message.message.end();

        while (it != end) {
            auto next = std::find(it, end, '\n');
            auto line = std::string_view{&*it, static_cast<size_t>(std::distance(it, next))};
            it = next;

            fmt::println("{} {}", header, line);

            if (it != end) {
                ++it;
            }
        }

        m_buffer.clear();
    }

public:
    constexpr ConsoleLog(IArena &arena, logs::Severity severity)
        : ILogger(severity)
        , m_buffer(arena) {}
};

class BroadcastLog final : public logs::ILogger {
    sm::Vector<logs::ILogger *> m_loggers;

    void accept(const logs::Message &message) override {
        for (ILogger *logger : m_loggers) {
            logger->log(message);
        }
    }

public:
    constexpr BroadcastLog(logs::Severity severity)
        : ILogger(severity) {}

    void add_logger(logs::ILogger *logger) {
        m_loggers.push_back(logger);
    }
};

class DefaultArena final : public IArena {
    using IArena::IArena;

    void *impl_alloc(size_t size) override {
        return std::malloc(size);
    }

    void *impl_resize(void *ptr, size_t new_size, size_t old_size) override {
        CT_UNUSED(old_size);

        return std::realloc(ptr, new_size);
    }

    void impl_release(void *ptr, size_t size) override {
        CT_UNUSED(size);

        std::free(ptr);
    }
};

class TraceArena final : public IArena {
    logs::Sink<logs::Category::eDebug> m_log;
    IArena &m_source;

    void *impl_alloc(size_t size) override {
        void *ptr = m_source.alloc(size);
        m_log.trace("[{}] alloc({:#x}) = {}\n", name, size, ptr);
        return ptr;
    }

    void *impl_resize(void *ptr, size_t new_size, size_t old_size) override {
        void *new_ptr = m_source.resize(ptr, new_size, old_size);
        m_log.trace("[{}] resize({}, {:#x}, {}) = {}\n", name, ptr, new_size, old_size, new_ptr);
        return new_ptr;
    }

    void impl_release(void *ptr, size_t size) override {
        m_source.release(ptr, size);
        m_log.trace("[{}] release({}, {:#x})\n", name, ptr, size);
    }

    void impl_rename(const void *ptr, const char *ptr_name) override {
        m_log.trace("[{}] rename({}, {})\n", name, ptr, ptr_name);
    }

    void impl_reparent(const void *ptr, const void *parent) override {
        m_log.trace("[{}] reparent({}, {})\n", name, ptr, parent);
    }

public:
    TraceArena(const char *name, IArena &source, logs::ILogger &logger)
        : IArena(name)
        , m_log(logger)
        , m_source(source) {}
};

static print_backtrace_t print_options_make(arena_t *arena, io_t *io) {
    print_backtrace_t print = {
        .options = {.arena = arena, .io = io, .pallete = &kColourDefault},
        .heading_style = eHeadingGeneric,
        .zero_indexed_lines = false,
        .project_source_path = SMC_SOURCE_DIR,
    };

    return print;
}

class DefaultSystemError final : public ISystemError {
    IArena &m_arena;
    bt_report_t *m_report = nullptr;

    void error_begin(os_error_t error) override {
        m_report = bt_report_new(&m_arena);
        io_t *io = io_stderr();
        io_printf(io, "System error detected: (%s)\n", os_error_string(error, &m_arena));
    }

    void error_frame(const bt_frame_t *it) override {
        bt_report_add(m_report, it);
    }

    void error_end() override {
        const print_backtrace_t kPrintOptions = print_options_make(&m_arena, io_stderr());
        print_backtrace(kPrintOptions, m_report);
        std::exit(CT_EXIT_INTERNAL); // NOLINT
    }

public:
    constexpr DefaultSystemError(IArena &arena)
        : m_arena(arena) {}
};

class DefaultWindowEvents final : public sys::IWindowEvents {
    sys::FileMapping &m_store;

    sys::WindowPlacement *m_placement = nullptr;
    sys::RecordLookup m_lookup;

    render::Context *m_context = nullptr;
    sys::DesktopInput *m_input = nullptr;

    LRESULT event(sys::Window &window, UINT message, WPARAM wparam, LPARAM lparam) override {
        if (m_input) m_input->window_event(message, wparam, lparam);
        return ImGui_ImplWin32_WndProcHandler(window.get_handle(), message, wparam, lparam);
    }

    void resize(sys::Window &, math::int2 size) override {
        if (m_context != nullptr) {
            m_context->resize(size.as<uint32_t>());
        }
    }

    void create(sys::Window &window) override {
        if (m_lookup = m_store.get_record(&m_placement); m_lookup == sys::RecordLookup::eOpened) {
            window.set_placement(*m_placement);
        } else {
            window.center_window(sys::MultiMonitor::ePrimary);
        }
    }

    bool close(sys::Window &window) override {
        if (m_lookup.has_valid_data()) *m_placement = window.get_placement();
        return true;
    }

public:
    DefaultWindowEvents(sys::FileMapping &store)
        : m_store(store) {}

    void attach_render(render::Context *context) {
        m_context = context;
    }

    void attach_input(sys::DesktopInput *input) {
        m_input = input;
    }
};

struct System {
    System(HINSTANCE hInstance, logs::ILogger &logger) {
        sys::create(hInstance, logger);
    }
    ~System() {
        sys::destroy();
    }
};

constinit static DefaultArena gGlobalArena{"default"};
constinit static DefaultSystemError gDefaultError{gGlobalArena};

static BroadcastLog gGlobalLog{logs::Severity::eInfo};

// i wont explain this
extern "C" HWND WINAPI GetConsoleWindow(void);
static bool is_console_app() {
    return GetConsoleWindow() != nullptr;
}

class ImGuiCommands : public render::IRenderNode {
    render::SrvHeapIndex imgui_index = render::SrvHeapIndex::eInvalid;

public:
    void create(render::Context &ctx) override {
        const auto &config = ctx.get_config();
        auto &srv_heap = ctx.get_srv_heap();
        imgui_index = srv_heap.bind_slot();

        auto format = rhi::get_data_format(config.swapchain_format);

        ImGui_ImplDX12_Init(ctx.get_rhi_device(), (int)config.swapchain_length, format,
                            srv_heap.get_heap(), srv_heap.cpu_handle(imgui_index),
                            srv_heap.gpu_handle(imgui_index));
    }

    void destroy(render::Context &ctx) override {
        ImGui_ImplDX12_Shutdown();

        auto &srv_heap = ctx.get_srv_heap();
        srv_heap.unbind_slot(imgui_index);
    }

    void execute(render::Context &ctx) override {
        auto &commands = ctx.get_direct_commands();
        ImGui_ImplDX12_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow();

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commands.get());
    }
};

static void common_init(void) {
    bt_init();
    os_init();

    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        io_t *io = is_console_app() ? io_stderr()
                                    : io_file("crash.log", eAccessWrite, &gGlobalArena);

        const print_backtrace_t kPrintOptions = print_options_make(&gGlobalArena, io);

        auto message = sm::vformat(msg, args);

        gGlobalLog.log(logs::Category::eGlobal, logs::Severity::ePanic, message.data());

        bt_report_t *report = bt_report_collect(&gGlobalArena);
        print_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };

    if (is_console_app())
        gGlobalLog.add_logger(new ConsoleLog{gGlobalArena, logs::Severity::eTrace});

    gGlobalLog.add_logger(
        new FileLog{gGlobalArena, io_file("client.log", eAccessWrite, &gGlobalArena)});
}

static void message_loop(sys::ShowWindow show, sys::FileMapping &store) {
#if 0
    const char *appname = sys::get_exe_path();
    char *appdir = str_directory(appname, &gGlobalArena);
    char *bundlename = str_format(&gGlobalArena, "%s\\bundle.zip", appdir);

    bundle::AssetBundle assets{bundlename, gConsoleLog};
#else
    // TODO: figure out why zlib is busted
    bundle::AssetBundle assets{"build\\bundle", gGlobalLog};
#endif

    auto &font = assets.load_font("fonts\\public-sans-regular.ttf");

    sys::WindowConfig window_config = {
        .mode = sys::WindowMode::eWindowed,
        .width = 1280,
        .height = 720,
        .title = "Priority Zero",
        .logger = gGlobalLog,
    };

    DefaultWindowEvents events{store};

    sys::Window window{window_config, &events};
    sys::DesktopInput desktop_input{window};

    constexpr unsigned kBufferCount = 2;

    render::RenderConfig render_config = {
        .dsv_heap_size = 8,
        .rtv_heap_size = 8,
        .cbv_heap_size = 256,

        .swapchain_length = kBufferCount,
        .swapchain_format = bundle::DataFormat::eRGBA8_UNORM,
        .window = window,
        .logger = gGlobalLog,
    };

    rhi::RenderConfig rhi_config = {
        .debug_flags = rhi::DebugFlags::mask(),

        .buffer_count = kBufferCount,
        .buffer_format = bundle::DataFormat::eRGBA8_UNORM,

        .rtv_heap_size = 8,

        .adapter_lookup = rhi::AdapterPreference::eDefault,
        .adapter_index = 0,
        .software_adapter = false,
        .feature_level = rhi::FeatureLevel::eLevel_11_0,

        .window = window,
        .logger = gGlobalLog,
    };

    {
        rhi::Factory render{rhi_config};

        window.show_window(show);

        events.attach_input(&desktop_input);

        constexpr ImGuiConfigFlags kIoFlags = ImGuiConfigFlags_NavEnableGamepad |
                                              ImGuiConfigFlags_NavEnableKeyboard |
                                              ImGuiConfigFlags_DockingEnable |
                                              ImGuiConfigFlags_ViewportsEnable;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= kIoFlags;

        ImGui::StyleColorsDark();
        const auto codepoints = std::to_array(
            U"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()");

        ui::FontAtlas atlas{font, {1024, 1024}, {codepoints.begin(), codepoints.end() - 1}};

        auto title_text = ui::TextWidget(atlas, u8"Priority Zero")
                              .align({ui::AlignH::eLeft, ui::AlignV::eTop})
                              .padding({90.f, 30.f, 30.f, 30.f})
                              .scale(3.f);

        auto play_text = ui::TextWidget(atlas, u8"Play")
                             .align({ui::AlignH::eLeft, ui::AlignV::eMiddle})
                             .focus_colour(ui::kColourBlack)
                             .colour(ui::kColourWhite)
                             .scale(1.f);

        auto options_text = ui::TextWidget(atlas, u8"Options")
                                .align({ui::AlignH::eLeft, ui::AlignV::eMiddle})
                                .focus_colour(ui::kColourBlack)
                                .colour(ui::kColourWhite)
                                .scale(1.f);

        auto quit_text = ui::TextWidget(atlas, u8"Quit")
                             .align({ui::AlignH::eLeft, ui::AlignV::eMiddle})
                             .focus_colour(ui::kColourBlack)
                             .colour(ui::kColourWhite)
                             .scale(1.f);

        auto license_text = ui::TextWidget(atlas, u8"Licenses")
                                .align({ui::AlignH::eRight, ui::AlignV::eBottom})
                                .focus_colour(ui::kColourBlack)
                                .colour(ui::kColourWhite)
                                .scale(1.f);

        auto play_box = ui::StyleWidget(play_text)
                            .colour(ui::kColourDarkGrey)
                            .focus_colour(ui::kColourLightGrey)
                            .padding({10.f, 10.f, 20.f, 10.f});

        auto options_box = ui::StyleWidget(options_text)
                               .colour(ui::kColourDarkGrey)
                               .focus_colour(ui::kColourLightGrey)
                               .padding({10.f, 10.f, 20.f, 10.f});

        auto quit_box = ui::StyleWidget(quit_text)
                            .colour(ui::kColourDarkGrey)
                            .focus_colour(ui::kColourLightGrey)
                            .padding({10.f, 10.f, 20.f, 10.f});

        auto license_box = ui::StyleWidget(license_text)
                               .colour(ui::kColourDarkGrey)
                               .focus_colour(ui::kColourLightGrey)
                               .padding({10.f, 10.f, 20.f, 10.f});

        auto lower_bar = ui::HStackWidget()
                             .align({ui::AlignH::eLeft, ui::AlignV::eBottom})
                             .spacing(25.f)
                             .padding({150.f, 10.f, 25.f, 25.f})
                             .justify(ui::Justify::eFollowAlign)
                             .add(play_box)
                             .add(options_box)
                             .add(quit_box)
                             .add(license_box);

        auto main_menu = ui::VStackWidget()
                             .padding({25.f, 25.f, 25.f, 25.f})
                             .spacing(25.f)
                             .add(title_text)
                             .add(lower_bar);

        auto zlib_license = ui::TextWidget(atlas, u8"zlib")
                                .align({ui::AlignH::eCenter, ui::AlignV::eTop})
                                .focus_colour(ui::kColourBlack)
                                .scale(1.f);

        auto agility_license = ui::TextWidget(atlas, u8"Agility SDK")
                                   .align({ui::AlignH::eCenter, ui::AlignV::eTop})
                                   .focus_colour(ui::kColourBlack)
                                   .scale(1.f);

        auto freetype_license = ui::TextWidget(atlas, u8"FreeType2")
                                    .align({ui::AlignH::eCenter, ui::AlignV::eTop})
                                    .focus_colour(ui::kColourBlack)
                                    .scale(1.f);

        auto harfbuzz_license = ui::TextWidget(atlas, u8"HarfBuzz")
                                    .align({ui::AlignH::eCenter, ui::AlignV::eTop})
                                    .focus_colour(ui::kColourBlack)
                                    .scale(1.f);

        auto libfmt_license = ui::TextWidget(atlas, u8"fmtlib")
                                  .align({ui::AlignH::eCenter, ui::AlignV::eTop})
                                  .focus_colour(ui::kColourBlack)
                                  .scale(1.f);

        auto cthulhu_license = ui::TextWidget(atlas, u8"Cthulhu Runtime")
                                   .align({ui::AlignH::eCenter, ui::AlignV::eTop})
                                   .focus_colour(ui::kColourBlack)
                                   .scale(1.f);

        auto publicsans_license = ui::TextWidget(atlas, u8"Public Sans")
                                      .align({ui::AlignH::eCenter, ui::AlignV::eTop})
                                      .focus_colour(ui::kColourBlack)
                                      .scale(1.f);

        auto license_list = ui::VStackWidget()
                                .padding({25.f, 25.f, 25.f, 25.f})
                                .spacing(25.f)
                                .add(zlib_license)
                                .add(agility_license)
                                .add(freetype_license)
                                .add(harfbuzz_license)
                                .add(libfmt_license)
                                .add(cthulhu_license)
                                .add(publicsans_license);

        // title_text.set_debug_draw(true, ui::kColourGreen);
        // play_text.set_debug_draw(true, ui::kColourGreen);
        // options_text.set_debug_draw(true, ui::kColourGreen);
        // quit_text.set_debug_draw(true, ui::kColourGreen);
        // license_text.set_debug_draw(true, ui::kColourGreen);

        // play_box.set_debug_draw(true, ui::kColourRed);
        // options_box.set_debug_draw(true, ui::kColourRed);
        // quit_box.set_debug_draw(true, ui::kColourRed);
        // license_box.set_debug_draw(true, ui::kColourRed);

        auto [left, top, right, bottom] = window.get_client_coords();

        ui::Canvas canvas{assets, atlas, &main_menu};
        input::InputService input;
        input.add_source(&desktop_input);

        auto width = unsigned(right - left);
        auto height = unsigned(bottom - top);

        canvas.set_user({50.f, 50.f}, {-50.f, -50.f});
        canvas.set_screen({width, height});

        ui::NavControl nav{canvas, &play_box};
        input.add_client(&nav);

        nav.add_bidi_link(&play_box, input::Button::eKeyRight, &options_box,
                          input::Button::eKeyLeft);
        nav.add_bidi_link(&options_box, input::Button::eKeyRight, &quit_box,
                          input::Button::eKeyLeft);
        nav.add_bidi_link(&quit_box, input::Button::eKeyRight, &license_box,
                          input::Button::eKeyLeft);

        nav.add_action(&license_list, input::Button::eEscape, [&] {
            canvas.set_root(&main_menu);
            nav.focus(&license_box);
        });

        nav.add_action(&license_box, input::Button::eSpace, [&] {
            canvas.set_root(&license_list);
            nav.focus(&license_list);
        });

        bool done = false;

        nav.add_action(&quit_box, input::Button::eSpace, [&] { done = true; });

        canvas.layout();

        // make imgui windows look more like native windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGuiStyle &style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        {
            render::Context context{render_config, render};
            events.attach_render(&context);
            ImGui_ImplWin32_Init(window.get_handle());

            // TODO: something here is leaking objects like mad
            // find it after demo 1 is done

            auto &cmd_imgui = context.add_node<ImGuiCommands>();
            auto &cmd_begin = context.add_node<render::BeginCommands>();
            auto &cmd_world = context.add_node<render::WorldCommands>(assets);
            auto &cmd_canvas = context.add_node<ui::CanvasCommands>(&canvas, assets);
            auto &cmd_end = context.add_node<render::EndCommands>();
            auto &cmd_present = context.add_node<render::PresentCommands>();

            // context.connect(cmd_present, cmd_end.render_target);

            while (!done) {
                // more complex message loop to avoid imgui
                // destroying the main window, then attempting to access it
                // in RenderPlatformWindowsDefault.
                // fun thing to debug
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

                ImGui_ImplWin32_NewFrame();

                context.execute_node(cmd_begin);

                context.execute_node(cmd_world);

                context.execute_node(cmd_canvas);

                context.execute_node(cmd_imgui);

                context.execute_node(cmd_end);

                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                    ImGui::UpdatePlatformWindows();

                    auto &commands = context.get_direct_commands();
                    ImGui::RenderPlatformWindowsDefault(
                        nullptr, commands.get()); // TODO: this should be part of the imgui pass
                }

                context.execute_node(cmd_present);
            }
        }

        ImGui_ImplWin32_Shutdown();
    }
}

static int common_main(sys::ShowWindow show) {
    GlobalSink general{gGlobalLog};
    general.info("SMC_DEBUG = {}", SMC_DEBUG);
    general.info("CTU_DEBUG = {}", CTU_DEBUG);

    TraceArena ft_arena{"freetype", gGlobalArena, gGlobalLog};
    service::init_freetype(&ft_arena, &gGlobalLog);

    ImGuiFreeType::SetAllocatorFunctions(
        [](size_t size, void *user_data) {
            auto *arena = static_cast<IArena *>(user_data);
            return arena->alloc(size);
        },
        [](void *ptr, void *user_data) {
            auto *arena = static_cast<IArena *>(user_data);
            return arena->release(ptr, 0);
        },
        &ft_arena);

    sys::MappingConfig store_config = {
        .path = "client.bin",
        .size = {1, Memory::eMegabytes},
        .record_count = 256,
        .logger = gGlobalLog,
    };

    sys::FileMapping store{store_config};

    threads::CpuGeometry geometry = threads::global_cpu_geometry(gGlobalLog);

    threads::SchedulerConfig thread_config = {
        .worker_count = 8,
        .process_priority = threads::PriorityClass::eNormal,
    };
    threads::Scheduler scheduler{thread_config, geometry, gGlobalLog};

    if (!store.is_valid()) {
        store.reset();
    }

    message_loop(show, store);

    service::deinit_freetype();
    return 0;
}

int main(int argc, const char **argv) {
    GlobalSink general{gGlobalLog};
    common_init();

    FormatBuffer args{gGlobalArena};
    auto it = std::back_inserter(args);
    fmt::format_to(it, "args[{}] = {{", argc);
    for (int i = 0; i < argc; ++i) {
        if (i != 0) fmt::format_to(it, ", ");
        fmt::format_to(it, "[{}] = \"{}\"", i, argv[i]);
    }
    fmt::format_to(it, "}}\0");

    general.info("{}", std::string_view{args.data(), args.size()});

    System sys{GetModuleHandleA(nullptr), gGlobalLog};

    return common_main(sys::ShowWindow::eShow);
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    GlobalSink general{gGlobalLog};
    common_init();

    general.info("lpCmdLine = {}", lpCmdLine);
    general.info("nShowCmd = {}", nShowCmd);

    System sys{hInstance, gGlobalLog};

    return common_main(sys::ShowWindow{nShowCmd});
}
