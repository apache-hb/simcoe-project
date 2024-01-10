rhi_api = configure_file(
    input : simcoe_api_config,
    output : 'rhi_api.hpp',
    configuration : { 'API': 'RHI' }
)

rhi_include = include_directories('.', 'include')

src = [
    'src/rhi.cpp'
]

simcoe_librhi = library('rhi', src,
    cpp_args : [ '-DSM_RHI_BUILD=1' ],
    include_directories : rhi_include,
    dependencies : [ simcoe_core ]
)

simcoe_rhi = declare_dependency(
    link_with : simcoe_librhi,
    include_directories : rhi_include,
    dependencies : [ simcoe_core ]
)
