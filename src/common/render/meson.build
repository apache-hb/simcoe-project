###
### rendering dependencies
###

dstorage = dependency('dstorage')
winpixruntime = dependency('winpixruntime')
nvapi = dependency('nvapi', required : get_option('nvapi_enable'))
ags = dependency('amd_ags', required : get_option('amd_ags_enable'))

###
### render feature configuration
###

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

###
### reflection data
###

render_include = include_directories('.', 'include')

src = [
    reflect.process('data/render.ct'),

    daocc.process('data/render.xml'),

    metacc.process('include/render/next/render.hpp'),

    metacc.process('include/render/graph/graph.hpp'),
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

###
### monolithic render core
###

src = [
    # core render classes
    'src/core/object.cpp',
    'src/core/instance.cpp',
    'src/core/error.cpp',
    'src/core/resource.cpp',
    'src/core/format.cpp',

    # utils
    'src/result.cpp',

    # base render classes
    'src/dstorage.cpp',

    # core render context
    'src/render.cpp',

    'src/descriptor_heap.cpp',

    'src/viewport.cpp',

    # render graph
    'src/graph/render_pass.cpp',
    'src/graph.cpp',

    # vendor
    'src/vendor/nvidia/nvapi.cpp',
    'src/vendor/amd/ags.cpp',

    # render next
    'src/next/state.cpp',
    'src/next/render.cpp',
    'src/next/context.cpp',
]

deps = [
    # internal libraries
    core, logs, system, math, archive,
    world, config, render_meta, db,

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
    'Setup render instance': 'test/instance.cpp',
    'Save render device data': 'test/store.cpp',

    'Create render next CoreContext': 'test/next/context.cpp',
    'Create render next CoreContext with debug features': 'test/next/context-debug.cpp',
}

foreach name, source : testcases
    exe = executable('test-render-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ render, coretest, dbtest ]
    )

    test(name, exe,
        suite : 'render',
        kwargs : testkwargs
    )
endforeach
