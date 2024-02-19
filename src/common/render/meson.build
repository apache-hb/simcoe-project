render_include = include_directories('.', 'include')

src = [
    'src/vendor.cpp',
    'src/result.cpp',
    'src/instance.cpp',
    'src/render.cpp',
    'src/object.cpp',
    'src/imgui.cpp',

    'src/blit.cpp',

    'src/camera.cpp',
    'src/draw.cpp',
    'src/primitives.cpp',

    reflect.process('data/render.ct'),
    reflect.process('data/draw.ct')
]

deps = [
    core, logs, system, math, archive,
    d3d12, d3dx12, d3d12ma, imgui
]

librender = library('render', src,
    cpp_args : [ '-DSM_RENDER_BUILD=1' ],
    include_directories : [ render_include, reflect_include ],
    dependencies : deps
)

render = declare_dependency(
    link_with : librender,
    include_directories : [ render_include, librender.private_dir_include() ],
    dependencies : deps
)
