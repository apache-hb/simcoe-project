service_api = configure_file(
    input : simcoe_api_config,
    output : 'service_api.hpp',
    configuration : { 'API': 'SERVICE' }
)

service_include = include_directories('.', 'include')

src = [
    'src/service.cpp'
]

simcoe_libservice = library('service', src,
    cpp_args : [ '-DSM_SERVICE_BUILD=1' ],
    include_directories : service_include,
    dependencies : [ simcoe_core ]
)

simcoe_service = declare_dependency(
    link_with : simcoe_libservice,
    include_directories : service_include,
    dependencies : [ simcoe_core ]
)
