render_include = include_directories('.', 'include')

src = [
    'src/framegraph.cpp',
    'src/render.cpp',
    'src/graph.cpp',
    'src/commands.cpp',
    'src/passes.cpp',
    reflect.process('data/render.ct')
]

deps = [ core, logs, system, math, rhi, bundle ]

librender = library('render', src,
    cpp_args : [ '-DSM_RENDER_BUILD=1' ],
    include_directories : [ render_include, reflect_include ],
    dependencies : deps
)

render = declare_dependency(
    link_with : librender,
    include_directories : [ render_include, reflect_include, librender.private_dir_include() ],
    dependencies : deps
)
