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

libsystem = library('system', src,
    cpp_args : [ '-DSM_SYSTEM_BUILD=1' ],
    include_directories : [ system_include, reflect_include, res_include ],
    dependencies : [ core, cthulhu.get_variable('os') ]
)

system = declare_dependency(
    link_with : libsystem,
    include_directories : [ system_include, reflect_include, libsystem.private_dir_include() ],
    dependencies : [ core ]
)
