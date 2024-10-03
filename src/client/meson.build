
src = files([
    'src/main.cpp'
])

deps = [
    core, system, structured_logs_sinks, threads, imgui, render, input, archive, config, draw,
    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('format_core'),
    cthulhu.get_variable('io'),
    tracy, packets
]

pch = meson.current_source_dir() / 'src/stdafx.hpp'
