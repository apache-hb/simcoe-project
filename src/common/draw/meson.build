draw_include = include_directories('.', 'include')

src = [
    'src/opaque.cpp',
    'src/blit.cpp',
    'src/camera.cpp',

    'src/ecs/systems.cpp',

    'src/ecs/forward_plus/copy_light_data.cpp',
    'src/ecs/forward_plus/depth_prepass.cpp',
    'src/ecs/forward_plus/light_binning.cpp',
    'src/ecs/forward_plus/opaque.cpp'
]

deps = [ render, input ]

libdraw = library('draw', src,
    cpp_args : [ '-DSM_DRAW_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ draw_include, 'src' ],
    dependencies : deps
)

draw = declare_dependency(
    link_with : libdraw,
    include_directories : draw_include,
    dependencies : deps
)
