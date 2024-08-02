
src = files([
    'src/main.cpp'
])

deps = [
    core, system, logs, threads, imgui, render_core, input, archive, config, draw,
    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('format_core'),
    cthulhu.get_variable('io'),
    tracy
]

pch = meson.current_source_dir() / 'src/stdafx.hpp'
