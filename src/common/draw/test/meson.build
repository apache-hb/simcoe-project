# awful
fs.copyfile(dstorage.get_variable('bindir') / 'dstorage.dll')
subdir('redist')
subdir('redist/d3d12')

testcases = {
    'Setup DrawContext': 'context.cpp',
    'DrawContext has correct cleanup': 'next/draw_context_cleanup.cpp',

    'Dear ImGui DrawContext setup': 'next/imgui_context.cpp',
    'Dear ImGui DrawContext setup without debug layer': 'next/imgui_context_no_debug_layer.cpp',
}

foreach name, source : testcases
    exe = executable('test-draw-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ render, draw, drawtest ]
    )

    test(name, exe,
        suite : 'draw',
        kwargs : testkwargs + { 'timeout' : 30 }
    )
endforeach
