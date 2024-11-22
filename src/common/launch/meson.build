launch_include = include_directories('.', 'include')

src = [
    'src/launch.cpp',
    'src/guiwindow.cpp',
]

deps = [
    core, logs, log_sinks,
    threads, system, db, config, net,
    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('format_core'),
    cthulhu.get_variable('io'),
    imgui,
]

if host_machine.system() == 'windows'
    deps += [ render_next, draw_next ]
    src += [ 'src/appwindow.cpp' ]
endif

liblaunch = library('launch', src,
    include_directories : [ launch_include ],
    dependencies : deps
)

launch_deps = [ core, db, imgui ]

if host_machine.system() == 'windows'
    launch_deps += [ draw_next ]
endif

launch = declare_dependency(
    link_with : liblaunch,
    include_directories : [ launch_include ],
    dependencies : launch_deps
)
