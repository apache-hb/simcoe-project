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

###
### tests
###

testcases = {
    'Structured logging database setup': 'test/setup.cpp',
}

foreach name, sources : testcases
    exe = executable('test-logs-' + name.to_lower().replace(' ', '-'), sources,
        dependencies : [ log_sinks, dbtest ]
    )

    test(name, exe,
        suite : 'logs',
        kwargs : testkwargs
    )
endforeach

###
### benchmarks
###

benchcases = {
    'Structured logging database throughput': 'benchmark/setup.cpp',
}

foreach name, sources : benchcases
    exe = executable('bench-logs-' + name.to_lower().replace(' ', '-'), sources,
        include_directories : liblog_sinks.private_dir_include(),
        dependencies : [ log_sinks, dbtest ]
    )

    benchmark(name, exe,
        suite : 'logs',
        kwargs : benchkwargs
    )
endforeach
