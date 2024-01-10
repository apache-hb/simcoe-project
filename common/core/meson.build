simcoe_api_config = files('data/simcoe_api.hpp.in')

core_api = configure_file(
    input : simcoe_api_config,
    output : 'core_api.hpp',
    configuration : { 'API': 'CORE' }
)

project_version = meson.project_version()
project_version_parts = project_version.split('.')
build_type = get_option('buildtype')
library_type = get_option('default_library')

simcoe_cdata = configuration_data()
simcoe_cdata.set('SMC_VERSION_PATCH', project_version_parts[2].to_int())
simcoe_cdata.set('SMC_VERSION_MINOR', project_version_parts[1].to_int())
simcoe_cdata.set('SMC_VERSION_MAJOR', project_version_parts[0].to_int())
simcoe_cdata.set_quoted('SMC_VERSION', project_version)

simcoe_cdata.set10('SMC_DEBUG', build_type.contains('debug'))
simcoe_cdata.set10('SMC_RELEASE', build_type.contains('release'))

simcoe_cdata.set10('SMC_LIBRARY_SHARED', library_type == 'shared')
simcoe_cdata.set10('SMC_LIBRARY_STATIC', library_type == 'static')

core_config = configure_file(
    output : 'simcoe_config.hpp',
    configuration : simcoe_cdata
)

core_include = include_directories('.', 'include')

src = [
    'src/arena.cpp',
    'src/panic.cpp'
]

simcoe_libcore = library('core', src,
    cpp_args : [ '-DSM_CORE_BUILD=1' ],
    include_directories : core_include
)

simcoe_core = declare_dependency(
    link_with : simcoe_libcore,
    include_directories : core_include
)
