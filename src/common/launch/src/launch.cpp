#include "launch/launch.hpp"

#include "fmt/ranges.h"
#include "logs/logger.hpp"
#include "logs/sinks/channels.hpp"

#include "threads/threads.hpp"
#include "system/system.hpp"

#include "db/environment.hpp"

#include "net/net.hpp"

#include "config/config.hpp"
#include "config/parse.hpp"

#include "core/macros.h"
#include "core/memory.h"

#include "io/console.h"
#include "io/io.h"

#include "format/backtrace.h"
#include "format/colour.h"
#include "threads/topology.hpp"

#include "core/defer.hpp"
#include "core/win32.hpp"
#include <shellapi.h>

using namespace sm;

namespace launch = sm::launch;
namespace db = sm::db;
namespace config = sm::config;

LOG_MESSAGE_CATEGORY(LaunchLog, "Launch");

static const config::Group kLoggingGroup {
    name = "Logging",
    desc = "Logging configuration",
};

static const opt<logs::TimerSource> kLoggingTimerSource {
    name = "log-timer",
    desc = "Detection method to use when creating the logger timestamp source",
    group = kLoggingGroup,
    init = logs::TimerSource::eAutoDetect,
    choices = {
        val(logs::TimerSource::eAutoDetect) = "auto",
        val(logs::TimerSource::eHighResolutionClock) = "qpc",
        val(logs::TimerSource::eInvariantTsc) = "rdtsc",
    }
};

class GlobalDatabaseEnv {
    db::Environment mEnv;

public:
    db::Connection connect(const db::ConnectionConfig& config) {
        return mEnv.connect(config);
    }

    GlobalDatabaseEnv(db::DbType type)
        : mEnv(db::Environment::create(type))
    { }
};

class LazyDatabaseEnv {
    db::DbType mType;
    db::ConnectionConfig mConfig;

    std::optional<db::Environment> mEnv;
    std::optional<db::Connection> mConnection;

    void lazySetup() {
        if (mEnv.has_value() && mConnection.has_value()) return;

        mEnv = db::Environment::create(mType);
        mConnection = mEnv->connect(mConfig);
    }

public:
    db::Connection& connect() {
        lazySetup();

        return mConnection.value();
    }

    db::Connection moveConnection() {
        lazySetup();

        return std::move(mConnection.value());
    }

    LazyDatabaseEnv(sm::db::DbType type, const sm::db::ConnectionConfig& config) noexcept
        : mType(type)
        , mConfig(config)
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

struct GlobalInfo {
    std::optional<int> exitCode;
};

static GlobalInfo gGlobalInfo;
static sm::UniquePtr<GlobalDatabaseEnv> gLoggingEnv;
static sm::UniquePtr<LazyDatabaseEnv> gInfoEnv;
static DefaultSystemError gDefaultError{};

static void setupThreadState() {
    // TODO: move to our own thread library which is aware of topology
    SetProcessAffinityMask(GetCurrentProcess(), 0b1111'1111'1111'1111);
}

static void setupCthulhuRuntime() {
    bt_init();
    os_init();

    // TODO: popup window for panics and system errors in release builds
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

        // TODO: block until logs are flushed

        os_exit(CT_EXIT_INTERNAL); // NOLINT
    };
}

static void setupDatabaseInfo(const launch::LaunchInfo& info) {
    gLoggingEnv = sm::makeUnique<GlobalDatabaseEnv>(info.logDbType);
    gInfoEnv = sm::makeUnique<LazyDatabaseEnv>(info.infoDbType, info.infoDbConfig);
}

static void setupLogging(const fs::path& path, const db::ConnectionConfig& config) {
    logs::create(logs::LoggingConfig {
        .timer = kLoggingTimerSource.getValue()
    });

    logs::sinks::create(gLoggingEnv->connect(config));

    logs::sinks::addConsoleChannel();
    logs::sinks::addDebugChannel();
    logs::sinks::addFileChannel(path);
}

static void setupSystem(HINSTANCE hInstance) {
    system::create(hInstance);
}

static void setupThreads(bool enabled) {
    if (!enabled) return;

    threads::create();
    threads::saveThreadInfo(gInfoEnv->connect());
}

static void setupNetwork(bool enabled) {
    if (!enabled) return;

    net::create();
}

launch::LaunchResult::~LaunchResult() noexcept try {
    net::destroy();
    threads::destroy();
    system::destroy();
    logs::sinks::destroy();
} catch (const std::exception& err) {
    LOG_ERROR(LaunchLog, "unhandled exception: {}", err.what());
} catch (...) {
    LOG_ERROR(LaunchLog, "unknown unhandled exception");
}

bool launch::LaunchResult::shouldExit() const noexcept {
    return gGlobalInfo.exitCode.has_value();
}

int launch::LaunchResult::exitCode() const noexcept {
    return gGlobalInfo.exitCode.value_or(0);
}

launch::LaunchResult launch::commonInit(HINSTANCE hInstance, const LaunchInfo& info) {
    setupThreadState();
    setupCthulhuRuntime();
    setupDatabaseInfo(info);
    setupLogging(info.logPath, info.logDbConfig);
    setupSystem(hInstance);
    setupThreads(info.threads);
    setupNetwork(info.network);

    return LaunchResult {};
}

static launch::LaunchResult commonMainInner(HINSTANCE hInstance, std::span<const char*> args, const launch::LaunchInfo& info) try {
    launch::LaunchResult result = commonInit(hInstance, info);

    LOG_INFO(LaunchLog, "args = [{}]", fmt::join(args, ", "));

    if (int err = sm::parseCommandLine(args.size(), args.data())) {
        gGlobalInfo.exitCode = (err == -1) ? 0 : err;
    }

    return result;
} catch (const std::exception& err) {
    LOG_ERROR(LaunchLog, "unhandled exception: {}", err.what());
    gGlobalInfo.exitCode = -1;

    return launch::LaunchResult {};
} catch (...) {
    LOG_ERROR(LaunchLog, "unknown unhandled exception");
    gGlobalInfo.exitCode = -1;

    return launch::LaunchResult {};
}

launch::LaunchResult launch::commonInitMain(int argc, const char **argv, const LaunchInfo& info) {
    std::span<const char*> args{argv, size_t(argc)};
    return commonMainInner(GetModuleHandleA(nullptr), args, info);
}

launch::LaunchResult launch::commonInitWinMain(HINSTANCE hInstance, int nShowCmd, const LaunchInfo& info) {
    int argc = 0;
    LPWSTR *argvw = CommandLineToArgvW(GetCommandLineW(), &argc);
    defer { LocalFree((void*)argvw); };

    std::vector<std::string> strings(argc);
    for (int i = 0; i < argc; ++i) {
        strings[i] = sm::narrow(argvw[i]);
    }

    std::vector<const char*> argv(argc);
    for (int i = 0; i < argc; ++i) {
        argv[i] = strings[i].c_str();
    }

    return commonMainInner(hInstance, argv, info);
}
