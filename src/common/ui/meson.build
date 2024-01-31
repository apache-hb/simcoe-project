ui_include = include_directories('.', 'include')

src = [
    'src/ui.cpp',
    'src/render.cpp',
    'src/atlas.cpp',
    'src/text.cpp',
    reflect.process('data/ui.ct')
]

deps = [ core, logs, math, bundle, render, stb, harfbuzz ]

libui = library('ui', src,
    cpp_args : [ '-DSM_UI_BUILD=1' ],
    include_directories : [ ui_include, reflect_include ],
    dependencies : deps
)

ui = declare_dependency(
    link_with : libui,
    include_directories : [ ui_include, reflect_include, libui.private_dir_include() ],
    dependencies : deps
)
