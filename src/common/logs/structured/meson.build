###
### public api and implementation
###

src = [
    'src/logging.cpp',
    'src/logger.cpp',

    'src/timer.cpp',
]

logs_include = include_directories('include')

deps = [ core, logs ]

libstructured_logs = library('structured_logs', src,
    cpp_args : [ '-DSM_STRUCTURED_LOGS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : logs_include,
    dependencies : deps
)

structured_logs = declare_dependency(
    link_with : libstructured_logs,
    include_directories : logs_include,
    dependencies : [ core, logs ]
)
