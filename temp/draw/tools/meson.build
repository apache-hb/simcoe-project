deps = [
    draw_next,
    filebrowser,
    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('format_core'),
]

args = cpp.get_supported_arguments([ '-Wno-c99-designator' ])

exe = executable('draw-imgui-context', 'imgui_context.cpp',
    dependencies : deps,
    cpp_args : args,
)

run_target('run-imgui-context', command : [ exe ])
