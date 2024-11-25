draw_include = include_directories('.', 'include')

###
### implementation
###

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
]

deps = [ render_core, input, logs ]

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
### next implementation
###

vic20_shader = custom_target('vic20-shader',
    input : 'data/vic20.hlsl',
    output : '@BASENAME@.cs.cso',
    command : [
        dxc, '/Wx', '/Ges',
        '-I@CURRENT_SOURCE_DIR@/include',
        '-Tcs_6_2', '-EcsMain',
        '-Fo@OUTPUT@', '@INPUT@',
        # '-enable-16bit-types',
        '/Zi', '-Qembed_debug'
    ]
)

blit_ps_shader = custom_target('blit-ps-shader',
    input : 'data/blit.hlsl',
    output : '@BASENAME@.ps.cso',
    command : [
        dxc, '/Wx', '/Ges',
        '-I@CURRENT_SOURCE_DIR@/include',
        '-Tps_6_2', '-EpsMain',
        '-Fo@OUTPUT@', '@INPUT@',
        '/Zi', '-Qembed_debug'
    ]
)

blit_vs_shader = custom_target('blit-vs-shader',
    input : 'data/blit.hlsl',
    output : '@BASENAME@.vs.cso',
    command : [
        dxc, '/Wx', '/Ges',
        '-I@CURRENT_SOURCE_DIR@/include',
        '-Tvs_6_2', '-EvsMain',
        '-Fo@OUTPUT@', '@INPUT@',
        '/Zi', '-Qembed_debug'
    ]
)

src = [
    # next draw
    'src/components/camera.cpp',

    # draw context resources
    'src/resources/imgui.cpp',
    'src/resources/compute.cpp',

    # next draw passes
    'src/passes/vic20.cpp',
    'src/passes/blit.cpp',
    'src/passes/opaque.cpp',
    'src/passes/forwardplus.cpp',

    # next draw contexts
    'src/next/context.cpp',
    'src/next/vic20.cpp',
]

deps = [ render_next, logs, imgui ]

libdraw_next = library('draw-next', src,
    cpp_args : [ '-DSM_DRAW_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ draw_include, 'src' ],
    dependencies : deps,
    link_depends : [ vic20_shader, blit_ps_shader, blit_vs_shader ]
)

draw_next = declare_dependency(
    link_with : libdraw_next,
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
    dependencies : [ draw_next, rendertest ]
)

drawtest = declare_dependency(
    link_with : libdrawtest,
    include_directories : '.',
    dependencies : [ draw_next, rendertest ]
)

subdir('test')
