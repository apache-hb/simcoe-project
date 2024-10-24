###
### implementation
###

draw_include = include_directories('.', 'include')

src = [
    'src/draw.cpp',

    'src/opaque.cpp',
    'src/blit.cpp',
    'src/camera.cpp',

    'src/ecs/systems.cpp',

    'src/ecs/forward_plus/copy_light_data.cpp',
    'src/ecs/forward_plus/depth_prepass.cpp',
    'src/ecs/forward_plus/light_binning.cpp',
    'src/ecs/forward_plus/opaque.cpp',

    # next draw
    'src/components/camera.cpp',

    # next draw passes
    'src/next/context.cpp',
    'src/next/blit.cpp',
    'src/next/opaque.cpp',
]

deps = [ render, input, logs, imgui ]

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

###
### tools
###

subdir('tools')

###
### test utils
###

libdrawtest = library('draw-test', 'test/draw_test_common.cpp',
    include_directories : '.',
    dependencies : [ draw, rendertest ]
)

drawtest = declare_dependency(
    link_with : libdrawtest,
    include_directories : '.',
    dependencies : [ draw, rendertest ]
)

###
### tests
###

subdir('test')
