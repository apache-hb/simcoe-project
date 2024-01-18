rhi_api = configure_file(
    input : api_header,
    output : 'sm_rhi_api.hpp',
    configuration : { 'MOD': 'RHI' }
)

rhi_include = include_directories('.', 'include')

src = [
    'src/rhi.cpp',
    reflect.process('data/rhi.ct')
]

simcoe_librhi = library('rhi', src,
    cpp_args : [ '-DSM_RHI_BUILD=1' ],
    include_directories : [ rhi_include, reflect_include ],
    dependencies : [ core ]
)

simcoe_rhi = declare_dependency(
    link_with : simcoe_librhi,
    include_directories : [ rhi_include, reflect_include ],
    dependencies : [ core ]
)
