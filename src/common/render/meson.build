
render_include = include_directories('.', 'include')

src = [
    'src/result.cpp',
    'src/instance.cpp',
    'src/render.cpp',
    'src/dstorage.cpp',
    'src/object.cpp',

    'src/blit.cpp',

    'src/gltf.cpp',

    'src/graph/render_pass.cpp',
    'src/graph.cpp',

    reflect.process('data/render.ct'),
    reflect.process('data/graph.ct')
]

deps = [
    # internal libraries
    core, logs, system, math, archive,
    world, config,

    # other vendors
    stb, fastgltf, tracy,

    # microsoft
    d3d12, d3dx12, d3d12ma, pix, dstorage,

    # hardware vendors
    nvapi
]

librender = library('render', src,
    cpp_args : [ '-DSM_RENDER_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ render_include, reflect_include, 'src' ],
    dependencies : deps
)

libvendor = shared_module('vendor', 'src/vendor.cpp', dependencies : core)

render = declare_dependency(
    link_with : librender,
    objects : libvendor.extract_all_objects(recursive : true),
    include_directories : [ render_include, librender.private_dir_include() ],
    dependencies : deps
)
