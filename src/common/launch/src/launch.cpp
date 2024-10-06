#include "launch/launch.hpp"

#include "threads/threads.hpp"
#include "system/system.hpp"
#include "logs/structured/channels.hpp"

#include "core/macros.h"
#include "core/memory.h"

#include "io/console.h"
#include "io/io.h"

#include "format/backtrace.h"
#include "format/colour.h"

namespace launch = sm::launch;
namespace db = sm::db;

struct LoggingDb {
    db::Environment env;

    db::Connection connect(const db::ConnectionConfig& info) {
        return env.connect(info);
    }

    LoggingDb(sm::db::DbType type)
        : env(db::Environment::create(type))
    { }
};

static fmt_backtrace_t newPrintOptions(io_t *io) {
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

class DefaultSystemError final : public sm::ISystemError {
    bt_report_t *mReport = nullptr;

    void error_begin(sm::OsError error) override {
        bt_update();

        mReport = bt_report_new(get_default_arena());
        io_t *io = io_stderr();
        io_printf(io, "System error detected: (%s)\n", error.toString().c_str());
    }

    void error_frame(bt_address_t it) override {
        bt_report_add(mReport, it);
    }

    void error_end() override {
        const fmt_backtrace_t kPrintOptions = newPrintOptions(io_stderr());
        fmt_backtrace(kPrintOptions, mReport);
        os_exit(CT_EXIT_INTERNAL); // NOLINT
    }
};

static sm::UniquePtr<LoggingDb> gLogging;
static DefaultSystemError gDefaultError{};

launch::LaunchCleanup::~LaunchCleanup() noexcept {
    threads::destroy();
    system::destroy();
    logs::structured::destroy();
}

launch::LaunchCleanup launch::commonInit(HINSTANCE hInstance, const LaunchInfo& info) {
    bt_init();
    os_init();

    // TODO: popup window for panics and system errors
    gSystemError = gDefaultError;

    gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
        bt_update();
        io_t *io = io_stderr();
        arena_t *arena = get_default_arena();

        const fmt_backtrace_t kPrintOptions = newPrintOptions(io);

        auto message = sm::vformat(msg, args);

        LOG_PANIC(GlobalLog, "{}", message);

        fmt::println(stderr, "{}", message);

        bt_report_t *report = bt_report_collect(arena);
        fmt_backtrace(kPrintOptions, report);

        os_exit(CT_EXIT_INTERNAL); // NOLINT
    };

    gLogging = sm::makeUnique<LoggingDb>(info.logDbType);
    logs::structured::create(gLogging->connect(info.logDbConfig));

    logs::structured::addConsoleChannel();
    logs::structured::addDebugChannel();
    logs::structured::addFileChannel(info.logPath);

    system::create(hInstance);
    threads::create();

    return LaunchCleanup {};
}