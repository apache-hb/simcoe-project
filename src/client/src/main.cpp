#include "core/arena.hpp"
#include "core/backtrace.hpp"
#include "core/format.hpp"
#include "core/reflect.hpp" // IWYU pragma: keep
#include "core/text.hpp"
#include "core/units.hpp"

#include "logs/logs.hpp"

#include "service/freetype.hpp"

#include "system/input.hpp"
#include "system/io.hpp"
#include "system/system.hpp"

#include "threads/threads.hpp"

#include "backtrace/backtrace.h"
#include "base/panic.h"
#include "format/backtrace.h"
#include "format/colour.h"
#include "io/console.h"
#include "io/io.h"
#include "std/str.h"

#include <consoleapi.h>

using namespace sm;

using GlobalSink = logs::Sink<logs::Category::eGlobal>;

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

#if 0
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam,
                                                             LPARAM lParam);
#endif

// TODO: clean up loggers

static std::string_view format_log(sm::FormatBuffer &buffer, const logs::Message &message,
                                   const char *colour, const char *reset) {
    // we dont have fmt/chrono.h because we use it in header only mode
    // so pull out the hours/minutes/seconds/milliseconds manually

    auto hours = (message.timestamp / (60 * 60 * 1000)) % 24;
    auto minutes = (message.timestamp / (60 * 1000)) % 60;
    auto seconds = (message.timestamp / 1000) % 60;
    auto milliseconds = message.timestamp % 1000;

    fmt::format_to(std::back_inserter(buffer), "{}[{}]{}[{:02}:{:02}:{:02}.{:03}] {}:", colour,
                   message.severity, reset, hours, minutes, seconds, milliseconds,
                   message.category);

    std::string_view header{buffer.data(), buffer.size()};

    return header;
}

class FileLog final : public logs::ILogger {
    sm::FormatBuffer m_buffer;
    io_t *io;

    void accept(const logs::Message &message) override {
        std::string_view header = format_log(m_buffer, message, "", "");

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

        std::string_view header = format_log(m_buffer, message, colour, reset);

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

    // render::Context *m_context = nullptr;
    sys::DesktopInput *m_input = nullptr;

    LRESULT event(sys::Window &window, UINT message, WPARAM wparam, LPARAM lparam) override {
        if (m_input) m_input->window_event(message, wparam, lparam);

        return 0;
        //return ImGui_ImplWin32_WndProcHandler(window.get_handle(), message, wparam, lparam);
    }

    void resize(sys::Window &, math::int2 size) override {
        // if (m_context != nullptr) {
        //     m_context->resize(size.as<uint32_t>());
        // }
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

    // void attach_render(render::Context *context) {
    //     m_context = context;
    // }

    void attach_input(sys::DesktopInput *input) {
        m_input = input;
    }
};

constinit static DefaultArena gGlobalArena{"default"};
constinit static DefaultSystemError gDefaultError{gGlobalArena};
static constinit ConsoleLog gConsoleLog{gGlobalArena, logs::Severity::eInfo};

struct System {
    System(HINSTANCE hInstance) {
        sys::create(hInstance, gConsoleLog);
    }
    ~System() {
        sys::destroy();
    }
};

// i wont explain this
extern "C" HWND WINAPI GetConsoleWindow(void);
static bool is_console_app() {
    return GetConsoleWindow() != nullptr;
}

static void common_init(void) {
    bt_init();
    os_init();

    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        io_t *io = is_console_app() ? io_stderr()
                                    : io_file("crash.log", eAccessWrite, &gGlobalArena);

        const print_backtrace_t kPrintOptions = print_options_make(&gGlobalArena, io);

        auto message = sm::vformat(msg, args);

        gConsoleLog.log(logs::Category::eGlobal, logs::Severity::ePanic, message.data());

        bt_report_t *report = bt_report_collect(&gGlobalArena);
        print_backtrace(kPrintOptions, report);

        std::exit(CT_EXIT_INTERNAL); // NOLINT
    };
}

static void message_loop(sys::ShowWindow show, sys::FileMapping &store) {
    sys::WindowConfig window_config = {
        .mode = sys::WindowMode::eWindowed,
        .width = 1280,
        .height = 720,
        .title = "Priority Zero",
        .logger = gConsoleLog,
    };

    DefaultWindowEvents events{store};

    sys::Window window{window_config, &events};
    sys::DesktopInput desktop_input{window};

    input::InputService input;
    input.add_source(&desktop_input);

    window.show_window(show);

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

        input.poll();
    }
}

static int common_main(sys::ShowWindow show) {
    GlobalSink general{gConsoleLog};
    general.info("SMC_DEBUG = {}", SMC_DEBUG);
    general.info("CTU_DEBUG = {}", CTU_DEBUG);

    TraceArena ft_arena{"freetype", gGlobalArena, gConsoleLog};
    service::init_freetype(&ft_arena, &gConsoleLog);

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

    message_loop(show, store);

    service::deinit_freetype();
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

    System sys{GetModuleHandleA(nullptr)};

    return common_main(sys::ShowWindow::eShow);
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    GlobalSink general{gConsoleLog};
    common_init();

    general.info("lpCmdLine = {}", lpCmdLine);
    general.info("nShowCmd = {}", nShowCmd);

    System sys{hInstance};

    return common_main(sys::ShowWindow{nShowCmd});
}
