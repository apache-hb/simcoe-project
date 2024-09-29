#include <catch2/catch_test_macros.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

#include "core/error.hpp"
#include "core/macros.h"
#include "core/memory.h"
#include "format/backtrace.h"
#include "format/colour.h"
#include "io/console.h"
#include "io/io.h"

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

class DefaultSystemError final : public sm::ISystemError {
    bt_report_t *mReport = nullptr;

    void error_begin(sm::OsError error) override {
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

constinit static DefaultSystemError gDefaultError{};

static const int kInstallTerminateHandler = [] noexcept {
    std::set_terminate([]() {
        CT_NEVER("Uncaught exception");
    });
    return 0;
}();

struct RunListener : public Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting(Catch::TestRunInfo const& testRunInfo) override {
        bt_init();
        os_init();

        gSystemError = gDefaultError;

        gPanicHandler = [](source_info_t info, const char *msg, va_list args) {
            io_t *io = io_stderr();
            arena_t *arena = get_default_arena();

            const fmt_backtrace_t kPrintOptions = print_options_make(io);

            bt_report_t *report = bt_report_collect(arena);
            fmt_backtrace(kPrintOptions, report);

            FAIL(sm::vformat(msg, args));
        };
    }
};

CATCH_REGISTER_LISTENER(RunListener) // NOLINT
