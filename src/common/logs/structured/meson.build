src = [
    'src/message.cpp',
    daocc.process('data/logs.xml')
]

deps = [ core, logs, orm, dao ]

libstructured_logs = library('structured_logs', src,
    cpp_args : [ '-DSM_STRUCTURED_LOGS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ '.', 'include', 'src' ],
    dependencies : deps
)

structured_logs = declare_dependency(
    link_with : libstructured_logs,
    include_directories : [ '.', 'include', ],
    dependencies : [ core, logs, orm ]
)

exe = executable('test-structured-logs-setup', 'test/setup.cpp',
    dependencies : [ structured_logs, coretest ]
)

test('Structured Logs Setup', exe,
    suite : 'structured_logs',
    kwargs : testkwargs
)
