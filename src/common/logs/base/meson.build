src = [
    metacc.process('include/logs/logs.hpp')
]

deps = [ core, meta ]

liblogs = library('logs', src,
    cpp_args : [ '-DSM_LOGS_BUILD=1' ],
    include_directories : [ '.', 'include' ],
    dependencies : deps
)

logs = declare_dependency(
    link_with : liblogs,
    include_directories : [ '.', 'include', liblogs.private_dir_include() ],
    dependencies : [ core, meta ]
)
