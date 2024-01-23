service_include = include_directories('.', 'include')

src = [
    'src/service.cpp'
]

libservice = library('service', src,
    cpp_args : [ '-DSM_SERVICE_BUILD=1' ],
    include_directories : service_include,
    dependencies : [ core ]
)

service = declare_dependency(
    link_with : libservice,
    include_directories : service_include,
    dependencies : [ core ]
)
