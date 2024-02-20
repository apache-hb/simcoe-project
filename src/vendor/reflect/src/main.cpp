#include "argparse/argparse.h"
#include "config/config.h"
#include "core/macros.h"
#include "cthulhu/events/events.h"
#include "format/colour.h"
#include "format/notify.h"
#include "io/console.h"
#include "io/io.h"
#include "memory/memory.h"
#include "notify/notify.h"
#include "os/core.h"
#include "ref/scan.h"

#include "interop/compile.h"

#include "ref/sema.hpp"
#include "setup/setup.h"

CT_BEGIN_API
#include "ref_bison.h" // IWYU pragma: keep
#include "ref_flex.h" // IWYU pragma: keep
CT_END_API

CT_CALLBACKS(kCallbacks, ref);

static const cfg_info_t kConfigInfo = {
    .name = "reflect",
    .brief = "Simcoe C++ reflection metadata generator",
};

static const char *const kOutputHeaderArgs[] = { "header", NULL };

static const cfg_info_t kOutputHeaderInfo = {
    .name = "header",
    .brief = "Output header file",
    .long_args = kOutputHeaderArgs,
};

static const char *const kOutputShaderFile[] = { "shader", NULL };

static const cfg_info_t kOutputShaderInfo = {
    .name = "shader",
    .brief = "Output shader file",
    .long_args = kOutputShaderFile,
};

static const char *const kOutputSourceArgs[] = { "source", NULL };

static const cfg_info_t kOutputSourceInfo = {
    .name = "source",
    .brief = "Output source file",
    .long_args = kOutputSourceArgs,
};

static const version_info_t kToolVersion = {
    .license = "LGPLv3",
    .desc = "Simcoe C++ reflection metadata generator",
    .author = "Temptation Games",
    .version = CT_NEW_VERSION(1, 0, 0),
};

static int check_reports(logger_t *logger, report_config_t config, const char *title)
{
    int err = text_report(logger_get_events(logger), config, title);
    logger_reset(logger);

    if (err != CT_EXIT_OK)
    {
        return err;
    }

    return 0;
}

#define CHECK_LOG(logger, fmt)                               \
    do                                                       \
    {                                                        \
        int err = check_reports(logger, report_config, fmt); \
        if (err != CT_EXIT_OK)                                  \
        {                                                    \
            return err;                                      \
        }                                                    \
    } while (0)

struct tool_t
{
    tool_t(logger_t *logger, arena_t *arena)
        : m_logger(logger)
        , m_arena(arena)
    {
        m_config = config_root(&kConfigInfo, m_arena);

        m_output_header = config_string(m_config, &kOutputHeaderInfo, "reflect.h");
        m_output_source = config_string(m_config, &kOutputSourceInfo, "reflect.cpp");
        m_output_shader = config_string(m_config, &kOutputShaderInfo, "reflect.hlsl");

        m_options = get_default_options(m_config);

        m_trees = vector_new(4, m_arena);

        m_extra.reports = m_logger;
    }

    tool_config_t get_config(io_t *io, int argc, const char **argv) const
    {
        tool_config_t cfg = {
            .arena = m_arena,
            .io = io,

            .group = m_config,
            .version = kToolVersion,

            .argc = argc,
            .argv = argv,
        };

        return cfg;
    }

    io_t *open_file(const char *path, os_access_t access, const diagnostic_t *diag)
    {
        io_t *io = io_file(path, access, m_arena);
        if (io_error_t err = io_error(io); err != 0)
        {
            msg_notify(m_logger, diag, get_builtin_node(), "failed to open '%s': %s", path,
                       os_error_string(err, m_arena));
            return nullptr;
        }

        return io;
    }

    ref_scan_t m_extra = {};

    void set_context(scan_t *scan)
    {
        scan_set_context(scan, &m_extra);
    }

    void process_import(const char *path)
    {
        io_t *io = open_file(path, eOsAccessRead, &kEvent_FailedToOpenSourceFile);
        if (io == nullptr) return;

        scan_t *scan = scan_io("simcoe", io, m_arena);
        set_context(scan);

        parse_result_t result = scan_buffer(scan, &kCallbacks);
        if (result.result != eParseOk) return;

        //m_sema.import_module((ref_ast_t *)result.tree);
    }

    ref_ast_t *process_file(const char *path)
    {
        io_t *io = open_file(path, eOsAccessRead, &kEvent_FailedToOpenSourceFile);
        if (io == nullptr) return nullptr;

        scan_t *scan = scan_io("simcoe", io, m_arena);
        set_context(scan);

        parse_result_t result = scan_buffer(scan, &kCallbacks);
        if (result.result != eParseOk) return nullptr;

        return (ref_ast_t *)result.tree;
    }

    int emit_output(ref_ast_t *ast, const char *file)
    {
        const char *header_path = cfg_string_value(m_output_header);
        io_t *header = open_file(header_path, eOsAccessWrite | eOsAccessTruncate, &kEvent_FailedToCreateOutputFile);
        if (header == nullptr) return CT_EXIT_ERROR;

        text_config_t text_config = {
            .config = {
                .zeroth_line = false,
            },
            .colours = &kColourDefault,
            .io = io_stdout(),
        };

        report_config_t report_config = {
            .max_errors = 20,
            .max_warnings = 20,

            .report_format = eTextSimple,
            .text_config = text_config,
        };

        m_sema.forward_module(ast);
        CHECK_LOG(m_logger, "forwarding");

        m_sema.resolve_all();
        CHECK_LOG(m_logger, "resolving");

        m_sema.emit_all(header, file);
        CHECK_LOG(m_logger, "emitting");

        return CT_EXIT_OK;
    }

    vector_t *m_trees = nullptr;

    logger_t *m_logger = nullptr;
    arena_t *m_arena = nullptr;

    cfg_group_t *m_config = nullptr;

    cfg_field_t *m_output_header = nullptr;
    cfg_field_t *m_output_source = nullptr;
    cfg_field_t *m_output_shader = nullptr;

    default_options_t m_options = {};
    refl::Sema m_sema { m_logger, m_arena };
};

int main(int argc, const char **argv)
{
    setup_global();

    io_t *con = io_stdout();
    arena_t *arena = get_global_arena();

    logger_t *logger = logger_new(arena);

    tool_t tool{ logger, arena };

    tool_config_t config = tool.get_config(con, argc, argv);

    ap_t *ap = ap_new(tool.m_config, arena);

    if (int err = parse_argparse(ap, tool.m_options, config); err == CT_EXIT_SHOULD_EXIT)
    {
        return CT_EXIT_OK;
    }

    vector_t *paths = ap_get_posargs(ap);

    text_config_t text_config = {
        .config = {
            .zeroth_line = false,
        },
        .colours = &kColourDefault,
        .io = con,
    };

    report_config_t report_config = {
        .max_errors = 20,
        .max_warnings = 20,

        .report_format = eTextSimple,
        .text_config = text_config,
    };

    size_t source_count = vector_len(paths);
    if (source_count != 1)
    {
        msg_notify(logger, &kEvent_ExactlyOneSourceFile, get_builtin_node(), "expected exactly one source file");
    }

    CHECK_LOG(logger, "initializing");

    const char *file = (const char *)vector_get(paths, 0);
    ref_ast_t *ast = tool.process_file(file);

    CHECK_LOG(logger, "processing");

    return tool.emit_output(ast, file);
}
