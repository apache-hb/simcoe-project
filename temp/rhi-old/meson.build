rhi_include = include_directories('.', 'include')

src = [
    # core rhi
    'src/rhi.cpp',

    # graphics and compute pipelines + root signatures
    'src/pipeline.cpp',

    # command list and queue
    'src/commands.cpp',
    reflect.process('data/rhi.ct')
]

deps = [ core, logs, system, bundle, math, d3d12, d3dx12 ]

librhi = library('rhi', src,
    cpp_args : [ '-DSM_RHI_BUILD=1' ],
    include_directories : [ rhi_include, reflect_include ],
    dependencies : [ deps, i18n ]
)

rhi = declare_dependency(
    link_with : librhi,
    include_directories : [ rhi_include, reflect_include, librhi.private_dir_include() ],
    dependencies : deps
)
