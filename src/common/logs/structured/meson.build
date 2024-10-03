src = [
    'src/logging.cpp',
    'src/logger.cpp'
]

deps = [ core, logs ]

libstructured_logs = library('structured_logs', src,
    cpp_args : [ '-DSM_STRUCTURED_LOGS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ '.', 'include', 'src' ],
    dependencies : deps
)

structured_logs = declare_dependency(
    link_with : libstructured_logs,
    include_directories : [ '.', 'include', ],
    dependencies : [ core, logs ]
)
