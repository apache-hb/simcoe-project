world_include = include_directories('.', 'include')

src = [
    'src/world.cpp',
    reflect.process('data/world.ct')
]

deps = [ core, logs, system, archive ]

libworld = library('world', src,
    cpp_args : [ '-DSM_WORLD_BUILD=1' ],
    include_directories : [ world_include ],
    dependencies : deps
)

world = declare_dependency(
    link_with : libworld,
    include_directories : [ world_include, libworld.private_dir_include() ],
    dependencies : deps
)