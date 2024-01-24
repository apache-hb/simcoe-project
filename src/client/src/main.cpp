#include "core/arena.hpp"
#include "core/backtrace.hpp"
#include "core/text.hpp"

#include "core/units.hpp"
#include "simcoe_config.h"
#include "system/io.hpp"
#include "system/system.hpp"
#include "rhi/rhi.hpp"
#include "logs/logs.hpp"

#include "base/panic.h"

#include "format/backtrace.h"
#include "format/colour.h"
#include "io/console.h"
#include "backtrace/backtrace.h"
#include "os/os.h"
#include <iterator>

using namespace sm;

template<ctu::Reflected T>
static constexpr auto enum_to_string(T value) {
    return ctu::reflect<T>().to_string(value);
}

class ConsoleLog final : public logs::ILogger {
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

    void accept(const logs::Message& message) override {
        const auto c = enum_to_string(message.category);
        const auto s = enum_to_string(message.severity);

        const auto pallete = &kColourDefault;

        const char *colour = colour_get(pallete, get_colour(message.severity));
        const char *reset = colour_reset(pallete);

        std::printf("%s[%s]%s %s: %.*s\n", colour, s.data(), reset, c.data(), (int)message.message.length(), message.message.data());
    }

public:
    constexpr ConsoleLog(logs::Severity severity)
        : ILogger(severity)
    { }
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
    IArena& m_source;

    void *impl_alloc(size_t size) override {
        void *ptr = m_source.alloc(size);
        std::printf("[%s] alloc(%zu) = %p\n", name, size, ptr);
        return ptr;
    }

    void *impl_resize(void *ptr, size_t new_size, size_t old_size) override {
        void *new_ptr = m_source.resize(ptr, new_size, old_size);
        std::printf("[%s] resize(0x%p, %zu, %zu) = 0x%p\n", name, ptr, new_size, old_size, new_ptr);
        return new_ptr;
    }

    void impl_release(void *ptr, size_t size) override {
        m_source.release(ptr, size);
        std::printf("[%s] release(0x%p, %zu)\n", name, ptr, size);
    }

    void impl_rename(const void *ptr, const char *ptr_name) override {
        std::printf("[%s] rename(0x%p, %s)\n", name, ptr, ptr_name);
    }

    void impl_reparent(const void *ptr, const void *parent) override {
        std::printf("[%s] reparent(0x%p, %p)\n", name, ptr, parent);
    }

public:
    TraceArena(const char *name, IArena& source)
        : IArena(name)
        , m_source(source)
    { }
};

static print_backtrace_t print_options_make(arena_t *arena, io_t *io) {
    print_backtrace_t print = {
        .options = {
            .arena = arena,
            .io = io,
            .pallete = &kColourDefault
        },
        .heading_style = eHeadingGeneric,
        .zero_indexed_lines = false,
    };

    return print;
}

class DefaultSystemError final : public ISystemError {
    arena_t *m_arena = nullptr;
    bt_report_t *m_report = nullptr;

    void error_begin(size_t error) override {
        m_report = bt_report_new(m_arena);
        std::printf("system_error(0x%zX)\n", error);
    }

    void error_frame(const bt_frame_t *it) override {
        bt_report_add(m_report, it);
    }

    void error_end(void) override {
        const print_backtrace_t kPrintOptions = print_options_make(m_arena, io_stderr());
        print_backtrace(kPrintOptions, m_report);
        std::exit(CT_EXIT_INTERNAL);
    }

public:
    DefaultSystemError(arena_t *arena)
        : m_arena(arena)
    { }
};

class DefaultWindowEvents final : public sys::IWindowEvents {
    sys::FileMapping& m_store;

    sys::WindowPlacement *m_placement = nullptr;
    sys::RecordLookup m_lookup;

    void create(sys::Window& window) override {
        if (m_lookup = m_store.get_record(&m_placement); m_lookup == sys::RecordLookup::eOpened) {
            window.set_placement(*m_placement);
        } else {
            window.center_window(sys::MultiMonitor::ePrimary);
        }
    }

    void destroy(sys::Window& window) override {
        window.destroy_window();
    }

    bool close(sys::Window& window) override {
        if (m_lookup.has_valid_data())
            *m_placement = window.get_placement();
        return true;
    }

public:
    DefaultWindowEvents(sys::FileMapping& store)
        : m_store(store)
    { }
};

struct System {
    System(HINSTANCE hInstance, logs::ILogger& logger) { sys::create(hInstance, logger); }
    ~System() { sys::destroy(); }
};

static DefaultArena gGlobalArena{"default"};
static TraceArena gTraceArena{"trace", gGlobalArena};
static DefaultSystemError gDefaultError{&gGlobalArena};
static ConsoleLog gConsoleLog{logs::Severity::eInfo};

static void common_init(void) {
    bt_init();
    os_init();

    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        const print_backtrace_t kPrintOptions = print_options_make(&gGlobalArena, io_stderr());

        auto message = sm::vformat(msg, args);

        gConsoleLog.log(logs::Category::eGlobal, logs::Severity::ePanic, message.data());

        std::printf("[%s:%zu] %s\npanic: %.*s\n", info.file, info.line, info.function, (int)message.size(), message.data());

        bt_report_t *report = bt_report_collect(&gGlobalArena);
        print_backtrace(kPrintOptions, report);
    };
}

static int common_main(sys::ShowWindow show) {
    logs::Sink<logs::Category::eGlobal> general { gConsoleLog };
    sys::MappingConfig store_config = {
        .path = "client.bin",
        .size = { 4, Memory::eMegabytes },
        .record_count = 256,
        .logger = gConsoleLog,
    };

    sys::FileMapping store { store_config };

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

    DefaultWindowEvents events { store };

    sys::Window window { window_config, &events };

    rhi::RenderConfig render_config = {
        .debug_flags = rhi::DebugFlags::mask(),

        .buffer_count = 2,

        .dsv_heap_size = 256,
        .rtv_heap_size = 256,
        .cbv_srv_uav_heap_size = 256,

        .adapter_lookup = rhi::AdapterPreference::eDefault,
        .adapter_index = 0,
        .software_adapter = true,
        .feature_level = rhi::FeatureLevel::eLevel_11_0,

        .window = window,
        .logger = gConsoleLog,
    };

    rhi::Context render { render_config };

    window.show_window(show);

    general.info("SMC_DEBUG = {}", SMC_DEBUG);
    general.info("CTU_DEBUG = {}", CTU_DEBUG);

    MSG msg = { };
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);

        render.begin_frame();

        render.end_frame();
    }

    return 0;
}

int main(int argc, const char **argv) {
    logs::Sink<logs::Category::eGlobal> general { gConsoleLog };
    common_init();

    std::string args;
    auto it = std::back_inserter(args);
    fmt::format_to(it, "args[{}] = {{", argc);
    for (int i = 0; i < argc; ++i) {
        if (i != 0) fmt::format_to(it, ", ");
        fmt::format_to(it, "[{}] = \"{}\"", i, argv[i]);
    }
    fmt::format_to(it, "}}\0");

    general.info("{}", args);

    System sys { GetModuleHandleA(nullptr), gConsoleLog };

    return common_main(sys::ShowWindow::eShow);
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    logs::Sink<logs::Category::eGlobal> general { gConsoleLog };
    common_init();

    general.info("lpCmdLine = {}", lpCmdLine);
    general.info("nShowCmd = {}", nShowCmd);

    System sys { hInstance, gConsoleLog };

    return common_main(sys::ShowWindow{nShowCmd});
}
