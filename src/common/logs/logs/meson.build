###
### reflection data
###

logs_include = include_directories('include')

src = [
    metacc.process('include/logs/logs.hpp'),
]

deps = [ core, meta ]

liblogs_meta = library('logs-meta', src,
    include_directories : logs_include,
    dependencies : deps
)

logs_meta = declare_dependency(
    link_with : liblogs_meta,
    include_directories : [ logs_include, liblogs_meta.private_dir_include() ],
    dependencies : [ core, meta ]
)

###
### public api and implementation
###

src = [
    'src/logging.cpp',
    'src/logger.cpp',

    'src/timer.cpp',
]

deps = [ core, logs_meta ]

liblogs = library('logs', src,
    cpp_args : [ '-DSM_LOGS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : logs_include,
    dependencies : deps
)

logs = declare_dependency(
    link_with : liblogs,
    include_directories : logs_include,
    dependencies : deps
)
