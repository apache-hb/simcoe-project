system_api = configure_file(
    input : api_header,
    output : 'sm_system_api.hpp',
    configuration : { 'MOD': 'SYSTEM' }
)

system_include = include_directories('.', 'include')

src = [
    'src/system.cpp',
    reflect.process('data/system.ct')
]

simcoe_libsystem = library('system', src,
    cpp_args : [ '-DSM_SYSTEM_BUILD=1' ],
    include_directories : system_include,
    dependencies : [ core ]
)

simcoe_system = declare_dependency(
    link_with : simcoe_libsystem,
    include_directories : system_include,
    dependencies : [ core ]
)
