system_include = include_directories('.', 'include')

# TODO: splash screens, do them all in d3d12 rather than trying to grok
# how gdi works. its quick enough to start a d3d12 software device and
# render using that
# 'src/splash.cpp',

src = [
    # shared internal variables
    'src/common.cpp',

    # setup/teardown
    'src/system.cpp',

    # windowing
    'src/window.cpp',

    # win32 input source
    'src/input.cpp',
    reflect.process('data/system.ct')
]

deps = [ core, logs, math, cthulhu.get_variable('os'), input ]

libsystem = library('system', src,
    cpp_args : [ '-DSM_SYSTEM_BUILD=1' ],
    include_directories : [ system_include, res_include ],
    dependencies : deps
)

system = declare_dependency(
    link_with : libsystem,
    include_directories : [ system_include, libsystem.private_dir_include() ],
    dependencies : deps
)
