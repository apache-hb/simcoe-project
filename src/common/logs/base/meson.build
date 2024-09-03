src = [
    'src/logs.cpp',
    'src/file.cpp',
    'src/debug.cpp',
    'src/console.cpp'
]

deps = [
    cthulhu.get_variable('format_core'),
    core,
]

liblogs = library('logs', src,
    cpp_args : [ '-DSM_LOGS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ '.', 'include', 'src' ],
    dependencies : deps
)

logs = declare_dependency(
    link_with : liblogs,
    include_directories : [ '.', 'include', ],
    dependencies : [ core ]
)
