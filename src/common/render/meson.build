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
    reflect.process('data/render.ct'),
    reflect.process('data/graph.ct'),

    daocc.process('data/render.xml'),
]

librender_meta = library('render-meta', src,
    include_directories : [ render_include, reflect_include ],
    dependencies : [ core, logs, system, math, archive, config ]
)

render_meta = declare_dependency(
    link_with : librender_meta,
    include_directories : [ render_include, librender_meta.private_dir_include() ],
    dependencies : [ core, logs, system, math, archive, config ]
)

src = [
    render_config,

    # utils
    'src/result.cpp',
    'src/object.cpp',

    # base render classes
    'src/instance.cpp',
    'src/dstorage.cpp',

    # core render context
    'src/render.cpp',

    'src/descriptor_heap.cpp',
    'src/resource.cpp',

    'src/viewport.cpp',
    'src/format.cpp',

    # render graph
    'src/graph/render_pass.cpp',
    'src/graph.cpp',

    # vendor
    'src/vendor/nvidia/nvapi.cpp',
    'src/vendor/amd/ags.cpp',
]

deps = [
    # internal libraries
    core, logs, system, math, archive,
    world, config, render_meta,

    # other vendors
    tracy, d3d12ma,

    # microsoft
    d3d12, d3dx12, dstorage,
    winpixruntime.partial_dependency(includes : true),

    # hardware vendors
    nvapi, ags
]

if pix_feature.allowed()
    deps += [ winpixruntime ]
endif

librender_core = library('rendercore', src,
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ render_include, reflect_include, 'src' ],
    dependencies : deps
)

render_core = declare_dependency(
    link_with : librender_core,
    include_directories : [ render_include ],
    dependencies : deps
)

libvendor = shared_module('vendor', 'src/vendor.cpp', dependencies : core)

render = declare_dependency(
    objects : libvendor.extract_all_objects(recursive : true),
    include_directories : [ render_include ],
    dependencies : [ deps, render_core ]
)

testcases = {
    'Setup Instance': 'test/instance.cpp',
}

foreach name, source : testcases
    exe = executable('test-render-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ render, coretest ]
    )

    test('Render ' + name, exe,
        suite : 'render',
        kwargs : testkwargs
    )
endforeach
