inc = include_directories('include')

deps = [ core, logs ]

src = [ 'src/config.cpp' ]

libconfig = library('config', src,
    cpp_args : [ '-DSM_CONFIG_BUILD=1' ],
    include_directories : inc,
    dependencies : deps
)

config = declare_dependency(
    link_with : libconfig,
    include_directories : inc,
    dependencies : deps
)
