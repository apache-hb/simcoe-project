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
    'src/storage.cpp',

    # windowing
    'src/window.cpp',

    # win32 input source
    'src/input.cpp',
    reflect.process('data/system.ct'),
]

deps = [ core, logs, math, cthulhu.get_variable('os'), input, config, wil ]

libsystem = library('system', src,
    cpp_args : [ '-DSM_SYSTEM_BUILD=1' ],
    include_directories : [ system_include, res_include ],
    dependencies : deps
)

system_core = declare_dependency(
    link_with : libsystem,
    include_directories : [ system_include, libsystem.private_dir_include() ],
    objects : libsystem.extract_all_objects(recursive : true),
    dependencies : deps
)

if is_windows
    delayimp = cc.find_library('delayimp')
    src = [
        'src/delayload.cpp',
        reflect.process('data/delayload.ct')
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
