src = [
    'src/channels.cpp',

    'src/common.cpp',

    'src/console.cpp',
    'src/debug.cpp',
    'src/file.cpp',
    'src/database.cpp',

    daocc.process('data/logs.xml')
]

deps = [ logs, db, dependency('concurrentqueue'), cthulhu.get_variable('format_core') ]

liblog_sinks = library('log-sinks', src,
    cpp_args : [ '-DSM_STRUCTURED_LOGS_SINKS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ '.', 'include', 'src' ],
    dependencies : deps
)

log_sinks = declare_dependency(
    link_with : liblog_sinks,
    include_directories : [ '.', 'include', ],
    dependencies : [ logs, db ]
)

exe = executable('test-logs-setup', 'test/setup.cpp',
    include_directories : liblog_sinks.private_dir_include(),
    dependencies : [ log_sinks, coretest, db ]
)

test('Structured logs setup', exe,
    suite : 'logs',
    kwargs : testkwargs
)

exe = executable('benchmark-logs-setup', 'benchmark/setup.cpp',
    dependencies : [ log_sinks, coretest ]
)

benchmark('Structured logs messages', exe,
    suite : 'logs',
    kwargs : benchkwargs
)
