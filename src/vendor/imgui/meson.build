src = [
    'imgui_demo.cpp',
    'imgui_draw.cpp',
    'imgui_widgets.cpp',
    'imgui_tables.cpp',
    'imgui.cpp',
    'backends/imgui_impl_dx12.cpp',
    'backends/imgui_impl_win32.cpp',
]

d3d12 = cpp.find_library('d3d12')
dxgi = cpp.find_library('dxgi')
dxguid = cpp.find_library('dxguid')

imgui_include = include_directories('.', 'backends')

imgui_deps = [ d3d12, dxgi, dxguid, core, math ]

libimgui = library('imgui', src,
    include_directories : imgui_include,
    dependencies : imgui_deps
)

imgui = declare_dependency(
    link_with : libimgui,
    include_directories : imgui_include,
    dependencies : imgui_deps
)
