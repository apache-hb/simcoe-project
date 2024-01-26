src = [
    'misc/freetype/imgui_freetype.cpp',
    'imgui_demo.cpp',
    'imgui_draw.cpp',
    'imgui_widgets.cpp',
    'imgui_tables.cpp',
    'imgui.cpp',

    # the backends we use
    'backends/imgui_impl_dx12.cpp',
    'backends/imgui_impl_win32.cpp',
]

dxgi = cpp.find_library('dxgi')
dxguid = cpp.find_library('dxguid')

imgui_include = include_directories('include')
private_imgui_include = include_directories('include/imgui', 'include/imgui/backends', 'include/imgui/misc')

imgui_deps = [ dxgi, core, math ]

libimgui = library('imgui', src,
    cpp_args : cpp.get_supported_arguments('/wd4702', '/wd4127', '-Wno-unused-function'),
    include_directories : [ imgui_include, private_imgui_include ],
    dependencies : imgui_deps + [ d3d12, dxguid, freetype2 ]
)

imgui = declare_dependency(
    link_with : libimgui,
    include_directories : imgui_include,
    dependencies : imgui_deps
)
