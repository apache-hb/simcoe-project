
render_include = include_directories('.', 'include')

nvapi = dependency('nvapi', required : get_option('nvapi_enable'))
ags = dependency('amd_ags', required : get_option('amd_ags_enable'))

render_cdata = configuration_data()
render_cdata.set10('SMC_RENDER_ENABLE_NVAPI', nvapi.found())
render_cdata.set10('SMC_RENDER_ENABLE_AMD_AGS', ags.found())
render_cdata.set10('SMC_RENDER_FRAMEGRAPH_TRACE', get_option('framegraph_trace'))

render_config = configure_file(
    output : 'simcoe_render_config.h',
    configuration : render_cdata
)

src = [
    render_config,

    # base render classes
    'src/instance.cpp',
    'src/dstorage.cpp',

    # core render context
    'src/render.cpp',

    # utils
    'src/result.cpp',
    'src/object.cpp',
    'src/descriptor_heap.cpp',
    'src/viewport.cpp',
    'src/format.cpp',

    # render graph
    'src/graph/render_pass.cpp',
    'src/graph.cpp',

    # vendor
    'src/vendor/nvidia/nvapi.cpp',
    'src/vendor/amd/ags.cpp',

    reflect.process('data/render.ct'),
    reflect.process('data/graph.ct')
]

deps = [
    # internal libraries
    core, logs, system, math, archive,
    world, config,

    # other vendors
    tracy,

    # microsoft
    d3d12, d3dx12, d3d12ma, pix, dstorage,

    # hardware vendors
    nvapi, ags
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
