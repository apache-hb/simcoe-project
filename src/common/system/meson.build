system_include = include_directories('.', 'include')

src = [
    'src/common.cpp',
    'src/system.cpp',
    'src/io.cpp',
    reflect.process('data/system.ct')
]

libsystem = library('system', src,
    cpp_args : [ '-DSM_SYSTEM_BUILD=1' ],
    include_directories : [ system_include, reflect_include, res_include ],
    dependencies : [ core, cthulhu.get_variable('os') ]
)

system = declare_dependency(
    link_with : libsystem,
    include_directories : [ system_include, reflect_include, libsystem.private_dir_include() ],
    dependencies : [ core ]
)
