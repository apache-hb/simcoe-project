logs_include = include_directories('.', 'include')

src = [
    'src/logs.cpp',
    'src/panic.cpp',
    reflect.process('data/logs.ct')
]

liblogs = library('logs', src,
    cpp_args : [ '-DSM_LOGS_BUILD=1' ],
    include_directories : [ logs_include ],
    dependencies : [ core ]
)

logs = declare_dependency(
    link_with : liblogs,
    include_directories : [ logs_include, liblogs.private_dir_include() ],
    dependencies : [ core ]
)
