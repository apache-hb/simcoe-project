fs.copyfile(dstorage.get_variable('bindir') / 'dstorage.dll')
subdir('redist')
subdir('redist/d3d12')

executable('draw-imgui-context', 'imgui_context.cpp',
    include_directories : draw_include,
    dependencies : [ draw ]
)
