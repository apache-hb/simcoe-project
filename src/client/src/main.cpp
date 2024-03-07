#include "archive/io.hpp"
#include "core/arena.hpp"
#include "core/error.hpp"
// #include "core/format.hpp"
#include "core/macros.h"
#include "core/format.hpp" // IWYU pragma: keep
#include "core/span.hpp"
#include "core/text.hpp"
#include "core/units.hpp"

// #include "archive/io.hpp"
#include "archive/record.hpp"
#include "system/input.hpp"
#include "system/system.hpp"

#include "system/timer.hpp"
#include "threads/threads.hpp"

#include "imgui/imgui.h"
// #include "imgui/backends/imgui_impl_win32.h"
// #include "imgui/backends/imgui_impl_dx12.h"

#include "render/render.hpp"

#include "backtrace/backtrace.h"
#include "base/panic.h"
#include "format/backtrace.h"
#include "format/colour.h"
#include "io/console.h"
#include "io/io.h"

#include "fmt/ranges.h"

using namespace sm;
using namespace math;

// void *operator new(size_t size) {
//     NEVER("operator new called");
// }

// void operator delete(void *ptr) {
//     NEVER("operator delete called");
// }

// void *operator new[](size_t size) {
//     NEVER("operator new[] called");
// }

// void operator delete[](void *ptr) {
//     NEVER("operator delete[] called");
// }

static auto gSink = logs::get_sink(logs::Category::eGlobal);

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
                   message.category);

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

#if 0
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

#endif

static print_backtrace_t print_options_make(io_t *io) {
    print_backtrace_t print = {
        .options = {.arena = sm::global_arena(), .io = io, .pallete = &kColourDefault},
        .heading_style = eHeadingGeneric,
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

    void error_frame(const bt_frame_t *it) override {
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
        gSink.info("create window");
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

        gSink.log(logs::Severity::ePanic, "{}", message);

        bt_report_t *report = bt_report_collect(arena);
        print_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };

    auto& logger = logs::get_logger();
    logger.add_channel(&gConsoleLog);
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
    events.attach_input(&desktop_input);
    input.add_source(&desktop_input);

    window.show_window(show);

    auto client = window.get_client_coords().size();

    IoHandle tar = io_file("build/bundle.tar", eOsAccessRead, sm::global_arena());
    sm::Bundle bundle{*tar, archive::BundleFormat::eTar};

    // enabling gpu based validation on the warp adapter
    // absolutely tanks performance
    constexpr render::DebugFlags flags = render::DebugFlags::eWarpAdapter
        | render::DebugFlags::eDeviceDebugLayer
        | render::DebugFlags::eFactoryDebug
        | render::DebugFlags::eDeviceRemovedInfo
        | render::DebugFlags::eInfoQueue
        | render::DebugFlags::eAutoName;

    const render::RenderConfig render_config = {
        .flags = flags,
        .preference = render::AdapterPreference::eMinimumPower,
        .feature_level = render::FeatureLevel::eLevel_11_0,
        .adapter_index = 0,

        .swapchain_length = 2,
        .swapchain_format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .swapchain_size = client.as<uint>(),

        .scene_format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .depth_format = DXGI_FORMAT_D32_FLOAT,
        .scene_size = { 1920, 1080 },

        .rtv_heap_size = 64,
        .dsv_heap_size = 64,
        .srv_heap_size = 1024,

        .bundle = bundle,
        .window = window,
    };

    render::Context context{render_config};

    events.attach_render(&context);
    input.add_client(&context.camera);

    context.create();

    sys::Ticker clock;

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

        if (context.update())
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

    threads::CpuGeometry geometry = threads::global_cpu_geometry();

    threads::SchedulerConfig thread_config = {
        .worker_count = 8,
        .process_priority = threads::PriorityClass::eNormal,
    };
    threads::Scheduler scheduler{thread_config, geometry};

    if (!store.is_valid()) {
        store.reset();
    }

    message_loop(show, store);

    return 0;
}

static int common_main(sys::ShowWindow show) {
    gSink.info("SMC_DEBUG = {}", SMC_DEBUG);
    gSink.info("CTU_DEBUG = {}", CTU_DEBUG);

    int result = client_main(show);

    gSink.info("client exiting with {}", result);

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
    gSink.info("args = [{}]", fmt::join(args, ", "));

    System sys{GetModuleHandleA(nullptr)};

    return common_main(sys::ShowWindow::eShow);
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    common_init();

    gSink.info("lpCmdLine = {}", lpCmdLine);
    gSink.info("nShowCmd = {}", nShowCmd);

    System sys{hInstance};

    return common_main(sys::ShowWindow{nShowCmd});
}
