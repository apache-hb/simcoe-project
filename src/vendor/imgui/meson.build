src = [
    'imgui_demo.cpp',
    'imgui_draw.cpp',
    'imgui_widgets.cpp',
    'imgui_tables.cpp',
    'imgui.cpp',

    # imguizmo requires some patches
    'imgui_backport.cpp',

    # stdlib integration
    'misc/cpp/imgui_stdlib.cpp',

    'backends/imgui_impl_glfw.cpp',
    'backends/imgui_impl_opengl3.cpp',

    # freetype integration
    # 'misc/freetype/imgui_freetype.cpp',
]

deps = [ core, math, dependency('glfw3'), dependency('GL') ]

if host_machine.system() == 'windows'
    src += [
        'backends/imgui_impl_dx12.cpp',
        'backends/imgui_impl_win32.cpp',
    ]
    deps += [ d3d12 ]
endif

args = cpp.get_supported_arguments('-Wno-nontrivial-memaccess')

imgui_include = include_directories('include')
private_imgui_include = include_directories('include/imgui', 'include/imgui/backends', 'include/imgui/misc')

libimgui = library('imgui', src,
    cpp_args : args + cpp.get_supported_arguments(
        '/wd4702', '/wd4127',
        '-Wno-unused-function',
        '-Wno-unused-variable',
        '-Wno-unused-but-set-variable'
    ),
    include_directories : [ imgui_include, private_imgui_include ],
    dependencies : deps
)

imgui = declare_dependency(
    link_with : libimgui,
    compile_args : args,
    include_directories : imgui_include,
    dependencies : deps
)
