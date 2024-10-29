exe = executable('draw-imgui-context', 'imgui_context.cpp',
    dependencies : [ draw_next ],
    cpp_args : cpp.get_supported_arguments([ '-Wno-c99-designator' ]),
)

run_target('run-imgui-context', command : [ exe ])
