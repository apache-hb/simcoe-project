launch_include = include_directories('.', 'include')

src = [
    'src/launch.cpp'
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

launch = declare_dependency(
    link_with : liblaunch,
    include_directories : [ launch_include ],
    dependencies : [ core ]
)
