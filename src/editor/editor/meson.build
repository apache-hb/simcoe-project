src = [
    'src/context.cpp',

    'src/mygui.cpp',
    'src/editor.cpp',

    'src/gltf.cpp',
    'src/image.cpp',

    'src/panels/assets.cpp',
    'src/panels/config.cpp',
    'src/panels/debug.cpp',
    'src/panels/features.cpp',
    'src/panels/graph.cpp',
    'src/panels/inspector.cpp',
    'src/panels/logger.cpp',
    'src/panels/panel.cpp',
    'src/panels/pix.cpp',
    'src/panels/scene.cpp',
    'src/panels/viewport.cpp',
    'src/panels/physics.cpp'
]

deps = [
    core, system, logs, threads, imgui, render,
    input, archive, config, draw, game,
    project, fastgltf,

    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('format_core'),
    cthulhu.get_variable('io'),
    tracy,

    dependency('stb_image'),

    imgui, implot, imguizmo, filebrowser,
    imnodes, imgui_memory_editor,

    dependency('directx-tex'),
    dependency('directx-mesh')
]

inc = include_directories('include')

libeditorgui = library('editor-gui', src,
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ inc, 'src' ],
    dependencies : deps,
)

editorgui = declare_dependency(
    include_directories : inc,
    link_with : libeditorgui,
    dependencies : deps
)
