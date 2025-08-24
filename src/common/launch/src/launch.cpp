#include "launch/launch.hpp"

#include "fmt/ranges.h"
#include "logs/logger.hpp"
#include "logs/appenders/channels.hpp"

#include "threads/threads.hpp"

#include "system/system.hpp"

#if _WIN32
#   include "threads/topology.hpp"
#   include <shellapi.h>
#endif

#include "db/environment.hpp"

#include "net/net.hpp"

#include "config/config.hpp"
#include "config/parse.hpp"

#include "core/error.hpp"
#include "core/string.hpp"

#include "core/macros.h"
#include "core/memory.h"

#include "io/console.h"
#include "io/io.h"

#include "format/backtrace.h"
#include "format/colour.h"

using namespace sm;

#ifndef _WIN32
#define GetModuleHandleA(x) nullptr
#endif

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
        io_printf(io, "System error detected: (%s)\n", error.what());
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
#if _WIN32
    // TODO: move to our own thread library which is aware of topology
    SetProcessAffinityMask(GetCurrentProcess(), 0b1111'1111'1111'1111);
#endif
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

    logs::appenders::create(gLoggingEnv->connect(config));

    logs::appenders::addConsoleChannel();
    logs::appenders::addDebugChannel();
    logs::appenders::addFileChannel(path);
}

static void setupSystem(HINSTANCE hInstance, bool enableCom) {
#if _WIN32
    system::create(hInstance);
#endif
}

static void setupThreads(bool enabled) {
    if (!enabled) return;

    threads::create();
#if _WIN32
    threads::saveThreadInfo(gInfoEnv->connect());
#endif
}

static void setupNetwork(bool enabled) {
    if (!enabled) return;

    net::create();
}

launch::LaunchResult::~LaunchResult() noexcept try {
    net::destroy();
    threads::destroy();
    system::destroy();
    logs::appenders::destroy();
} catch (const errors::AnyException& err) {
    LOG_ERROR(GlobalLog, "Unhandled exception during launch: {}.", err.what());
    for (const auto& frame : err.stacktrace()) {
        LOG_ERROR(GlobalLog, "{}:{} - {}", frame.source_file(), frame.source_line(), frame.description());
    }
} catch (const std::exception& err) {
    LOG_ERROR(LaunchLog, "Unhandled exception during launch: {}.", err.what());
} catch (...) {
    LOG_ERROR(LaunchLog, "Unknown unhandled exception.");
}

bool launch::LaunchResult::shouldExit() const noexcept {
    return gGlobalInfo.exitCode.has_value();
}

int launch::LaunchResult::exitCode() const noexcept {
    return gGlobalInfo.exitCode.value_or(0);
}

db::Connection& launch::LaunchResult::getInfoDb() noexcept {
    return gInfoEnv->connect();
}

launch::LaunchResult launch::commonInit(HINSTANCE hInstance, const LaunchInfo& info) {
    setupThreadState();
    setupCthulhuRuntime();
    setupDatabaseInfo(info);
    setupLogging(info.logPath, info.logDbConfig);
    setupSystem(hInstance, info.com);
    setupThreads(info.threads);
    setupNetwork(info.network);

    return LaunchResult {};
}

static launch::LaunchResult commonMainInner(HINSTANCE hInstance, std::span<const char*> args, const launch::LaunchInfo& info) noexcept try {
    launch::LaunchResult result = commonInit(hInstance, info);

    LOG_INFO(LaunchLog, "Arguments = [{}]", fmt::join(args, ", "));

    if (int err = sm::parseCommandLine(args.size(), args.data())) {
        if (!info.allowInvalidArgs) {
            gGlobalInfo.exitCode = (err == -1) ? 0 : err;
        }
    }

    return result;
} catch (const errors::AnyException& err) {
    LOG_ERROR(GlobalLog, "Exception during launch: {}", err.what());
    for (const auto& frame : err.stacktrace()) {
        LOG_ERROR(GlobalLog, "{}:{} - {}", frame.source_file(), frame.source_line(), frame.description());
    }
    gGlobalInfo.exitCode = -1;

    return launch::LaunchResult {};
} catch (const std::exception& err) {
    LOG_ERROR(GlobalLog, "{}", err.what());
    gGlobalInfo.exitCode = -1;

    return launch::LaunchResult {};
} catch (...) {
    LOG_ERROR(GlobalLog, "Unknown unhandled exception.");
    gGlobalInfo.exitCode = -1;

    return launch::LaunchResult {};
}

launch::LaunchResult launch::commonInitMain(int argc, const char **argv, const LaunchInfo& info) noexcept {
    std::span<const char*> args{argv, size_t(argc)};
    return commonMainInner(GetModuleHandleA(nullptr), args, info);
}

launch::LaunchResult launch::commonInitWinMain(HINSTANCE hInstance, int nShowCmd, const LaunchInfo& info) noexcept {
    std::vector<std::string> strings = system::getCommandLine();

    std::vector<const char*> argv(strings.size());
    for (size_t i = 0; i < strings.size(); ++i) {
        argv[i] = strings[i].c_str();
    }

    return commonMainInner(hInstance, argv, info);
}
