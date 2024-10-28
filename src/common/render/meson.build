###
### rendering dependencies
###

wil = dependency('wil')
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
agility_feature = get_option('agility_enable')

render_cdata = configuration_data()
render_cdata.set10('SMC_RENDER_ENABLE_NVAPI', nvapi.found())
render_cdata.set10('SMC_RENDER_ENABLE_AMD_AGS', ags.found())
render_cdata.set10('SMC_RENDER_FRAMEGRAPH_TRACE', framegraph_trace_feature.allowed())
render_cdata.set10('SMC_RENDER_ENABLE_WINPIXRUNTIME', pix_feature.allowed())
render_cdata.set10('SMC_RENDER_ENABLE_WARP', warp_feature.allowed())
render_cdata.set10('SMC_RENDER_ENABLE_AGILITY', agility_feature.allowed())

render_config = configure_file(
    output : 'simcoe_render_config.h',
    configuration : render_cdata
)

###
### reflection data
###

render_include = include_directories('.', 'include')

deps = [
    core, logs, system, math, archive, config,
    d3d12, d3dx12,
]

src = [
    reflect.process('data/render.ct'),

    daocc.process('data/render.xml'),

    metacc.process('include/render/next/render.hpp'),

    metacc.process('include/render/graph/graph.hpp'),
]

librender_meta = library('render-meta', src,
    include_directories : [ render_include, reflect_include ],
    dependencies : deps
)

render_meta = declare_dependency(
    link_with : librender_meta,
    include_directories : [ render_include, librender_meta.private_dir_include() ],
    dependencies : deps
)

###
### core render objects
###

src = [
    'src/core/object.cpp',
    'src/core/instance.cpp',
    'src/core/error.cpp',
    'src/core/resource.cpp',
    'src/core/format.cpp',
]

deps = [
    # reflection
    render_meta,

    # internal libraries
    core, logs, math, db, world,

    wil,

    # microsoft d3d12
    d3d12, d3dx12,
    dstorage,
    winpixruntime.partial_dependency(includes : true),

    # hardware vendors
    nvapi, ags,

    # other vendors
    tracy, d3d12ma,
]

librender_base = library('render-base', src,
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ render_include, 'src' ],
    dependencies : deps,
)

render_base = declare_dependency(
    link_with : librender_base,
    include_directories : [ render_include ],
    dependencies : deps
)

###
### symbol exports
###

lib_export_ihv = shared_module('render-export-ihv', 'src/exports/ihv.cpp', dependencies : core)
lib_export_dstorage = shared_module('render-export-dstorage', 'src/exports/dstorage.cpp', dependencies : core)
lib_export_agility = shared_module('render-export-agility', 'src/exports/agility.cpp', dependencies : core)

ihv_export = declare_dependency(
    objects : lib_export_ihv.extract_all_objects(recursive : true)
)

dstorage_export = declare_dependency(
    objects : lib_export_dstorage.extract_all_objects(recursive : true)
)

agility_export = declare_dependency(
    objects : lib_export_agility.extract_all_objects(recursive : true)
)

###
### monolithic render core
###

src = [
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
]

deps = [
    # internal libraries
    core, logs, system, math, archive,
    world, config,

    render_base,

    # other vendors
    tracy, d3d12ma,

    # microsoft
    d3d12, d3dx12, dstorage,
    winpixruntime.partial_dependency(includes : true),

    # hardware vendors
    nvapi, ags,

    # exports
    ihv_export, dstorage_export, agility_export,
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

###
### next render
###

src = [
    # render next
    'src/next/state.cpp',
    'src/next/render.cpp',

    # render context and resources tied to it
    'src/next/context/core.cpp',
    'src/next/context/resource.cpp',

    # components
    'src/next/components/components.cpp',
    'src/next/components/viewport.cpp',
    'src/next/components/fence.cpp',
    'src/next/components/commands.cpp',
    'src/next/components/descriptors.cpp',

    # swapchain factories
    'src/next/surface/surface.cpp',
    'src/next/surface/virtual.cpp',
    'src/next/surface/swapchain.cpp',
]

deps = [
    # internal libraries
    core, logs, system, math, archive,
    world, config,

    render_base,

    # other vendors
    d3d12ma,

    # microsoft
    d3d12, d3dx12,

    # exports
    ihv_export
]

librender_next = library('render-next', src,
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ render_include, reflect_include, 'src' ],
    dependencies : deps
)

render_next = declare_dependency(
    link_with : librender_next,
    include_directories : [ render_include ],
    dependencies : deps
)

subdir('test')
