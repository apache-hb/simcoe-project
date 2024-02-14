render_include = include_directories('.', 'include')

src = [
    'src/vendor.cpp',
    'src/common.cpp',
    'src/context.cpp',
    'src/instance.cpp',

    reflect.process('data/render.ct')
]

deps = [
    core, logs, system, math,
    d3d12, d3dx12, d3d12ma, i18n
]

librender = library('render', src,
    cpp_args : [ '-DSM_RENDER_BUILD=1' ],
    include_directories : [ render_include, reflect_include ],
    dependencies : deps
)

render = declare_dependency(
    link_with : librender,
    include_directories : [ render_include, librender.private_dir_include() ],
    dependencies : deps
)
