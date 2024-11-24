world_include = include_directories('.', 'include')

src = [
    'src/mesh.cpp',
    'src/world.cpp',
    'src/ecs.cpp',
]

deps = [
    core, logs, system, archive,
    d3d12, d3dx12,
    dependency('flecs')
]

libworld = library('world', src,
    cpp_args : [ '-DSM_WORLD_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ world_include, reflect_include ],
    dependencies : deps
)

world = declare_dependency(
    link_with : libworld,
    include_directories : [ world_include, reflect_include ],
    dependencies : deps
)
