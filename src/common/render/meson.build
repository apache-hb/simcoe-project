render_include = include_directories('.', 'include')

dstorage = dependency('dstorage')
winpixruntime = dependency('winpixruntime')
nvapi = dependency('nvapi', required : get_option('nvapi_enable'))
ags = dependency('amd_ags', required : get_option('amd_ags_enable'))

framegraph_trace_feature = get_option('framegraph_trace').disable_auto_if(is_release)
warp_feature = get_option('warp_enable').disable_auto_if(is_release)
pix_feature = get_option('pix_enable').disable_auto_if(is_release)

render_cdata = configuration_data()
render_cdata.set10('SMC_RENDER_ENABLE_NVAPI', nvapi.found())
render_cdata.set10('SMC_RENDER_ENABLE_AMD_AGS', ags.found())
render_cdata.set10('SMC_RENDER_FRAMEGRAPH_TRACE', framegraph_trace_feature.allowed())
render_cdata.set10('SMC_RENDER_ENABLE_WINPIXRUNTIME', pix_feature.allowed())
render_cdata.set10('SMC_RENDER_ENABLE_WARP', warp_feature.allowed())

render_config = configure_file(
    output : 'simcoe_render_config.h',
    configuration : render_cdata
)

src = [
    render_config,

    # utils
    'src/core/result.cpp',
    'src/core/object.cpp',

    # base render classes
    'src/core/instance.cpp',
    'src/core/dstorage.cpp',

    # core render context
    'src/core/render.cpp',

    'src/core/descriptor_heap.cpp',
    'src/core/resource.cpp',

    'src/core/viewport.cpp',
    'src/core/format.cpp',

    # render graph
    'src/core/graph/render_pass.cpp',
    'src/core/graph.cpp',

    # vendor
    'src/core/vendor/nvidia/nvapi.cpp',
    'src/core/vendor/amd/ags.cpp',

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
    d3d12, d3dx12, d3d12ma, dstorage,
    winpixruntime.partial_dependency(includes : true),

    # hardware vendors
    nvapi, ags
]

if pix_feature.allowed()
    deps += [ winpixruntime ]
endif

librender_core = library('render-core', src,
    cpp_pch : 'src/core/stdafx.hpp',
    include_directories : [ render_include, reflect_include, 'src/core' ],
    dependencies : deps
)

libvendor = shared_module('vendor', 'src/core/vendor.cpp', dependencies : core)

render_core = declare_dependency(
    link_with : librender_core,
    objects : libvendor.extract_all_objects(recursive : true),
    include_directories : [ render_include, librender_core.private_dir_include() ],
    dependencies : deps
)
