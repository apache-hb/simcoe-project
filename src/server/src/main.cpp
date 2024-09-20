#include "stdafx.hpp"

#include "db/transaction.hpp"
#include "db/connection.hpp"

#include "threads/threads.hpp"

#include "logs/structured/message.hpp"

using namespace sm;

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

static DefaultSystemError gDefaultError{};
static logs::FileChannel gFileChannel{};

struct LogWrapper {
    db::Environment env;
    db::Connection connection;

    LogWrapper()
        : env(db::Environment::create(db::DbType::eSqlite3))
        , connection(env.connect({ .host = "logs.db" }))
    {
        sm::logs::structured::setup(connection).throwIfFailed();
    }

    ~LogWrapper() {
        sm::logs::structured::cleanup();
    }
};

sm::UniquePtr<LogWrapper> gLogWrapper;

static void commonInit(void) {
    // bt_init();
    os_init();

    // TODO: popup window for panics and system errors
    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        bt_update();
        io_t *io = io_stderr();
        arena_t *arena = get_default_arena();

        const fmt_backtrace_t kPrintOptions = print_options_make(io);

        auto message = sm::vformat(msg, args);

        fmt::println(stderr, "{}", message);

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

    gLogWrapper = sm::makeUnique<LogWrapper>();

    threads::init();
}

static int serverMain() {
    const threads::CpuGeometry& geometry = threads::getCpuGeometry();

    threads::SchedulerConfig threadConfig = {
        .workers = 8,
        .priority = threads::PriorityClass::eNormal,
    };
    threads::Scheduler scheduler{threadConfig, geometry};

    return 0;
}

static int commonMain() noexcept try {
    logs::gGlobal.info("SMC_DEBUG = {}", SMC_DEBUG);
    logs::gGlobal.info("CTU_DEBUG = {}", CTU_DEBUG);

    int result = serverMain();

    return result;
} catch (const std::exception& err) {
    logs::gGlobal.error("unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    logs::gGlobal.error("unknown unhandled exception");
    return -1;
}

int main(int argc, const char **argv) noexcept try {
    commonInit();
    defer { gLogWrapper.reset(); };

    sm::Span<const char*> args{argv, size_t(argc)};
    logs::gGlobal.info("args = [{}]", fmt::join(args, ", "));

    int result = [&] {
        sys::create(GetModuleHandleA(nullptr));
        defer { sys::destroy(); };

        if (int err = sm::parseCommandLine(argc, argv)) {
            return (err == -1) ? 0 : err; // TODO: this is a little silly, should wrap in a type
        }

        return commonMain();
    }();

    logs::gGlobal.info("editor exiting with {}", result);

    logs::shutdown();

    return result;
} catch (const db::DbException& err) {
    logs::gGlobal.error("database error: {} ({})", err.what(), err.code());
    return -1;
} catch (const std::exception& err) {
    logs::gGlobal.error("unhandled exception: {}", err.what());
    return -1;
} catch (...) {
    logs::gGlobal.error("unknown unhandled exception");
    return -1;
}

int WinMain(HINSTANCE hInstance, SM_UNUSED HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    commonInit();
    defer { gLogWrapper.reset(); };

    logs::gGlobal.info("lpCmdLine = {}", lpCmdLine);
    logs::gGlobal.info("nShowCmd = {}", nShowCmd);
    // TODO: parse lpCmdLine

    int result = [&] {
        sys::create(hInstance);
        defer { sys::destroy(); };

        return commonMain();
    }();

    logs::gGlobal.info("editor exiting with {}", result);

    logs::shutdown();

    return result;
}
