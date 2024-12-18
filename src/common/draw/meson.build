draw_include = include_directories('.', 'include')

###
### implementation
###

shader_kwarg = {
    'install': true,
    'install_tag': 'runtime',
    'install_dir': get_option('bindir')
}

vic20_shader = custom_target('vic20-shader',
    input : 'data/vic20.hlsl',
    output : '@BASENAME@.cs.cso',
    command : [
        dxc, '/Wx', '/Ges',
        '-I@CURRENT_SOURCE_DIR@/include',
        '-Tcs_6_2', '-EcsMain',
        '-Fo@OUTPUT@', '@INPUT@',
        '/Zi', '-Qembed_debug'
    ],
    kwargs : shader_kwarg,
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
    ],
    kwargs : shader_kwarg,
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
    ],
    kwargs : shader_kwarg,
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

deps = [ render, logs, imgui ]

libdraw = library('draw', src,
    cpp_args : [ '-DSM_DRAW_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ draw_include, 'src' ],
    dependencies : deps,
    link_depends : [ vic20_shader, blit_ps_shader, blit_vs_shader ]
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

subdir('test')
