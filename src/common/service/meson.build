service_api = configure_file(
    input : api_header,
    output : 'sm_service_api.hpp',
    configuration : { 'MOD': 'SERVICE' }
)

service_include = include_directories('.', 'include')

src = [
    'src/service.cpp'
]

libservice = library('service', src,
    install : true,
    cpp_args : [ '-DSM_SERVICE_BUILD=1' ],
    include_directories : service_include,
    dependencies : [ core ]
)

service = declare_dependency(
    link_with : libservice,
    include_directories : service_include,
    dependencies : [ core ]
)
