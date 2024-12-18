launch_include = include_directories('.', 'include')

src = [
    'src/launch.cpp',
]

deps = [
    core, logs, log_sinks,
    threads, system, db, config, net,
    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('format_core'),
    cthulhu.get_variable('io'),
]

liblaunch = library('launch', src,
    include_directories : [ launch_include ],
    dependencies : deps
)

launch_deps = [ core, db ]

launch = declare_dependency(
    link_with : liblaunch,
    include_directories : [ launch_include ],
    dependencies : launch_deps
)

deps = [ launch, imgui ]

src = [
    'src/guiwindow.cpp',
]

if host_machine.system() == 'windows'
    deps += [ render, draw ]
    src += [ 'src/appwindow.cpp' ]
endif

liblaunch_gui = library('launch-gui', src,
    include_directories : [ launch_include ],
    dependencies : deps
)

launch_gui = declare_dependency(
    link_with : liblaunch_gui,
    include_directories : [ launch_include ],
    dependencies : deps
)
