system_include = include_directories('include')

src = [
    daocc.process('data/system.xml')
]

deps = [ core, db ]

libsystem_meta = library('system-meta', src,
    include_directories : [ system_include ],
    dependencies : deps
)

system_meta = declare_dependency(
    link_with : libsystem_meta,
    include_directories : [ system_include, libsystem_meta.private_dir_include() ],
    dependencies : deps
)

# TODO: splash screens, do them all in d3d12 rather than trying to grok
# how gdi works. its quick enough to start a d3d12 software device and
# render using that
# 'src/splash.cpp',

src = [ 'src/window.cpp' ]

deps = [
    core, logs, math,
    cthulhu.get_variable('os'),
    input, config,
    system_meta,
    dependency('glfw3')
]

if host_machine.system() == 'windows'
    src += [
        'src/common.cpp',
        'src/system.cpp',
        'src/input.cpp',
        'src/registry.cpp',

        'src/win32/storage.cpp',
        'src/win32/network.cpp',
        'src/win32/system.cpp',
    ]
    deps += [ dependency('wil'), cc.find_library('ws2_32') ]
elif host_machine.system() == 'linux'
    src += [
        'src/posix/storage.cpp',
        'src/posix/network.cpp',
        'src/posix/system.cpp',
    ]
endif

libsystem = library('system', src,
    cpp_args : [ '-DSM_SYSTEM_BUILD=1' ],
    include_directories : [ system_include, res_include ],
    dependencies : deps
)

system_core = declare_dependency(
    link_with : libsystem,
    include_directories : [ system_include ],
    objects : libsystem.extract_all_objects(recursive : true),
    dependencies : deps
)

if is_windows
    delayimp = cc.find_library('delayimp')
    src = [
        'src/delayload.cpp'
    ]

    libdelayload = shared_module('delayload', src,
        dependencies : [ system_core, delayimp ],
        include_directories : [ system_include ]
    )

    objs = libdelayload.extract_all_objects(recursive : true)
    deps += [ delayimp ]
else
    objs = []
endif

system = declare_dependency(
    link_with : libsystem,
    include_directories : [ system_include, libsystem.private_dir_include() ],
    objects : objs,
    dependencies : deps
)
