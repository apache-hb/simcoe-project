system_api = configure_file(
    input : simcoe_api_config,
    output : 'system_api.hpp',
    configuration : { 'API': 'SYSTEM' }
)

system_include = include_directories('.', 'include')

src = [
    'src/system.cpp'
]

simcoe_libsystem = library('system', src,
    cpp_args : [ '-DSM_SYSTEM_BUILD=1' ],
    include_directories : system_include,
    dependencies : [ simcoe_core ]
)

simcoe_system = declare_dependency(
    link_with : simcoe_libsystem,
    include_directories : system_include,
    dependencies : [ simcoe_core ]
)
