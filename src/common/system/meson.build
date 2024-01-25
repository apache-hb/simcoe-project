system_include = include_directories('.', 'include')

# TODO: splash screens, do them all in d3d12 rather than trying to grok
# how gdi works. its quick enough to start a d3d12 software device and
# render using that
# 'src/splash.cpp',

src = [
    'src/common.cpp',
    'src/system.cpp',
    'src/io.cpp',
    reflect.process('data/system.ct')
]

libsystem = library('system', src,
    cpp_args : [ '-DSM_SYSTEM_BUILD=1' ],
    include_directories : [ system_include, res_include ],
    dependencies : [ core, logs, cthulhu.get_variable('os') ]
)

system = declare_dependency(
    link_with : libsystem,
    include_directories : [ system_include, libsystem.private_dir_include() ],
    dependencies : [ core, logs ]
)
