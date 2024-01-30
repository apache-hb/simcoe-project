#include "core/arena.hpp"
#include "core/backtrace.hpp"
#include "core/text.hpp"

#include "core/units.hpp"
#include "logs/logs.hpp"
#include "rhi/rhi.hpp"
#include "service/freetype.hpp"
#include "system/io.hpp"
#include "system/system.hpp"
#include "threads/threads.hpp"


#include "base/panic.h"

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


#include <iterator>

using namespace sm;

using FormatBuffer = fmt::basic_memory_buffer<char, 256, sm::StandardArena<char>>;
using GlobalSink = logs::Sink<logs::Category::eGlobal>;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);

template <ctu::Reflected T>
static constexpr auto enum_to_string(T value) {
    constexpr auto refl = ctu::reflect<T>();
    return refl.to_string(value);
}

class ConsoleLog final : public logs::ILogger {
    FormatBuffer m_buffer;

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
        m_log.trace("[{}] resize({:#x}, {:#x}, {}) = 0x%p\n", name, ptr, new_size, old_size,
                    new_ptr);
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

    void error_begin(size_t error) override {
        m_report = bt_report_new(&m_arena);
        std::printf("system_error(0x%zX)\n", error);
    }

    void error_frame(const bt_frame_t *it) override {
        bt_report_add(m_report, it);
    }

    void error_end(void) override {
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

    LRESULT event(sys::Window &window, UINT message, WPARAM wparam, LPARAM lparam) override {
        return ImGui_ImplWin32_WndProcHandler(window.get_handle(), message, wparam, lparam);
    }

    void resize(sys::Window &, math::int2 size) override {
        if (m_context != nullptr) m_context->resize(size.as<uint32_t>());
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
constinit static ConsoleLog gConsoleLog{gGlobalArena, logs::Severity::eInfo};

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

    void build(render::Context &ctx) override {
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
        const print_backtrace_t kPrintOptions = print_options_make(&gGlobalArena, io_stderr());

        auto message = sm::vformat(msg, args);

        gConsoleLog.log(logs::Category::eGlobal, logs::Severity::ePanic, message.data());

        bt_report_t *report = bt_report_collect(&gGlobalArena);
        print_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };
}

static int common_main(sys::ShowWindow show) {
    GlobalSink general{gConsoleLog};
    general.info("SMC_DEBUG = {}", SMC_DEBUG);
    general.info("CTU_DEBUG = {}", CTU_DEBUG);

    TraceArena trace_freetype_arena{"freetype", gGlobalArena, gConsoleLog};
    service::init_freetype(&gConsoleLog);

    service::FreeType freetype{&trace_freetype_arena};

    ImGuiFreeType::SetAllocatorFunctions(
        [](size_t size, void *user_data) {
            auto *arena = static_cast<IArena *>(user_data);
            return arena->alloc(size);
        },
        [](void *ptr, void *user_data) {
            auto *arena = static_cast<IArena *>(user_data);
            return arena->release(ptr, 0);
        },
        &trace_freetype_arena);

    sys::MappingConfig store_config = {
        .path = "client.bin",
        .size = {1, Memory::eMegabytes},
        .record_count = 256,
        .logger = gConsoleLog,
    };

    sys::FileMapping store{store_config};

    threads::CpuGeometry geometry = threads::global_cpu_geometry(gConsoleLog);

    threads::SchedulerConfig thread_config = {
        .worker_count = 8,
        .process_priority = threads::PriorityClass::eNormal,
    };
    threads::Scheduler scheduler{thread_config, geometry, gConsoleLog};

    if (!store.is_valid()) {
        store.reset();
    }

    sys::WindowConfig window_config = {
        .mode = sys::WindowMode::eWindowed,
        .width = 1280,
        .height = 720,
        .title = "Priority Zero",
        .logger = gConsoleLog,
    };

    DefaultWindowEvents events{store};

    sys::Window window{window_config, &events};

    constexpr unsigned kBufferCount = 2;

    render::RenderConfig render_config = {
        .dsv_heap_size = 8,
        .rtv_heap_size = 8,
        .cbv_heap_size = 256,

        .swapchain_length = kBufferCount,
        .swapchain_format = bundle::DataFormat::eRGBA8_UNORM,
        .window = window,
        .logger = gConsoleLog,
    };

    rhi::RenderConfig rhi_config = {
        .debug_flags = rhi::DebugFlags::mask(),

        .buffer_count = kBufferCount,
        .buffer_format = bundle::DataFormat::eRGBA8_UNORM,

        .rtv_heap_size = 8,

        .adapter_lookup = rhi::AdapterPreference::eDefault,
        .adapter_index = 0,
        .software_adapter = true,
        .feature_level = rhi::FeatureLevel::eLevel_11_0,

        .window = window,
        .logger = gConsoleLog,
    };

    rhi::Factory render{rhi_config};

    window.show_window(show);

    constexpr ImGuiConfigFlags kIoFlags = ImGuiConfigFlags_NavEnableGamepad |
                                          ImGuiConfigFlags_NavEnableKeyboard |
                                          ImGuiConfigFlags_DockingEnable |
                                          ImGuiConfigFlags_ViewportsEnable;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= kIoFlags;

    ImGui::StyleColorsDark();

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

        auto &cmd_imgui = context.add_node<ImGuiCommands>();
        auto &cmd_begin = context.add_node<render::BeginCommands>();
        auto &cmd_world = context.add_node<render::WorldCommands>();
        auto &cmd_end = context.add_node<render::EndCommands>();
        auto &cmd_present = context.add_node<render::PresentCommands>();

        // context.connect(cmd_present, cmd_end.render_target);

        bool done = false;
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

            ImGui_ImplWin32_NewFrame();

            context.execute_node(cmd_begin);

            context.execute_node(cmd_world);

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

        // TODO: graph should manage resources
        context.destroy_node(cmd_imgui);
        context.destroy_node(cmd_world);

        ImGui_ImplWin32_Shutdown();
    }

    return 0;
}

int main(int argc, const char **argv) {
    GlobalSink general{gConsoleLog};
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

    System sys{GetModuleHandleA(nullptr), gConsoleLog};

    return common_main(sys::ShowWindow::eShow);
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    GlobalSink general{gConsoleLog};
    common_init();

    general.info("lpCmdLine = {}", lpCmdLine);
    general.info("nShowCmd = {}", nShowCmd);

    System sys{hInstance, gConsoleLog};

    return common_main(sys::ShowWindow{nShowCmd});
}
