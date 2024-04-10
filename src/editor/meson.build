
src = files([
    'src/main.cpp',
    'src/context.cpp',

    'src/mygui.cpp',
    'src/editor.cpp',

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
])

src += [
    reflect.process('data/editor.ct')
]

deps = [
    core, system, logs, threads, imgui, render,
    input, archive, config, draw, game,
    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('format_core'),
    cthulhu.get_variable('io'),
    tracy,

    imgui, implot, imguizmo, filebrowser,
    imnodes,

    dependency('directx-tex'),
    dependency('directx-mesh')
]

inc = [ include_directories('include', 'src'), reflect_include ]
pch = meson.current_source_dir() / 'src/stdafx.hpp'
