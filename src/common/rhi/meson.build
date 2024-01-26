rhi_include = include_directories('.', 'include')

src = [
    'src/rhi.cpp',
    reflect.process('data/rhi.ct')
]

deps = [ core, logs, system, math, d3d12, d3dx12 ]

librhi = library('rhi', src,
    cpp_args : [ '-DSM_RHI_BUILD=1' ],
    include_directories : [ rhi_include, reflect_include ],
    dependencies : deps
)

rhi = declare_dependency(
    link_with : librhi,
    include_directories : [ rhi_include, reflect_include, librhi.private_dir_include() ],
    dependencies : deps
)
