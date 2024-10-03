src = [
    'src/channels.cpp',

    'src/console.cpp',
    'src/file.cpp',
    'src/database.cpp',

    daocc.process('data/logs.xml')
]

deps = [ structured_logs, db, dependency('concurrentqueue') ]

libstructured_logs_sinks = library('structured_logs_sinks', src,
    cpp_args : [ '-DSM_STRUCTURED_LOGS_SINKS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ '.', 'include', 'src' ],
    dependencies : deps
)

structured_logs_sinks = declare_dependency(
    link_with : libstructured_logs_sinks,
    include_directories : [ '.', 'include', ],
    dependencies : [ structured_logs, db ]
)

exe = executable('test-structured-logs-setup', 'test/setup.cpp',
    dependencies : [ structured_logs_sinks, coretest ]
)

test('Structured logs setup', exe,
    suite : 'structured_logs',
    kwargs : testkwargs
)

exe = executable('benchmark-structured-logs-setup', 'benchmark/setup.cpp',
    dependencies : [ structured_logs_sinks, coretest ]
)

benchmark('Structured logs messages', exe,
    suite : 'structured_logs',
    kwargs : benchkwargs
)
