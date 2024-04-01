draw_include = include_directories('.', 'include')

src = [
    'src/scene.cpp',
    'src/blit.cpp',
    'src/camera.cpp'
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
