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

    'src/graph.cpp',

    'src/draw/scene.cpp',
    'src/draw/present.cpp',
    'src/draw/imgui.cpp',
    'src/draw/canvas.cpp',

    'src/editor/panel.cpp',
    'src/editor/logger.cpp',
    'src/editor/editor.cpp',
    'src/editor/config.cpp',
    'src/editor/scene.cpp',
    'src/editor/graph.cpp',
    'src/editor/features.cpp',
    'src/editor/inspector.cpp',
    'src/editor/gizmo.cpp',
    'src/editor/viewport.cpp',

    reflect.process('data/render.ct'),
    reflect.process('data/draw.ct'),
    reflect.process('data/graph.ct')
]

deps = [
    core, logs, system, math, archive,
    d3d12, d3dx12, d3d12ma, imgui,
    dds_texture_loader, world, stb,
    world, implot, imguizmo
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
