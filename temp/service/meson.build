service_include = include_directories('.', 'include')

src = [
    'src/service.cpp',
    'src/freetype.cpp'
]

service_deps = [ core, logs, freetype2 ]

libservice = library('service', src,
    cpp_args : [ '-DSM_SERVICE_BUILD=1' ],
    include_directories : service_include,
    dependencies : service_deps
)

service = declare_dependency(
    link_with : libservice,
    include_directories : service_include,
    dependencies : service_deps
)
