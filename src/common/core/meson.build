# we use the api header elsewhere so use files()
api_header = files('data/simcoe_api.hpp.in')

# generate the project config header
version = meson.project_version()
parts = version.split('.')
build_type = get_option('buildtype')
library_type = get_option('default_library')

simcoe_cdata = configuration_data()
simcoe_cdata.set('SMC_VERSION_PATCH', parts[2].to_int())
simcoe_cdata.set('SMC_VERSION_MINOR', parts[1].to_int())
simcoe_cdata.set('SMC_VERSION_MAJOR', parts[0].to_int())
simcoe_cdata.set_quoted('SMC_VERSION', version)

simcoe_cdata.set10('SMC_DEBUG', build_type.contains('debug'))
simcoe_cdata.set10('SMC_RELEASE', build_type.contains('release'))

simcoe_cdata.set10('SMC_BUILD_SHARED', library_type == 'shared')
simcoe_cdata.set10('SMC_BUILD_STATIC', library_type == 'static')

simcoe_config = configure_file(
    output : 'simcoe_config.hpp',
    configuration : simcoe_cdata
)

core_api = configure_file(
    input : api_header,
    output : 'sm_core_api.hpp',
    configuration : { 'MOD': 'CORE' }
)

core_include = include_directories('.', 'include')

core_dependencies = [
    cthulhu.get_variable('core'),
    cthulhu.get_variable('base'),
    cthulhu.get_variable('arena'),
    cthulhu.get_variable('std'),
    cthulhu.get_variable('backtrace')
]

src = [
    'src/arena.cpp',
    'src/text.cpp',
    'src/backtrace.cpp'
]

libcore = library('core', src,
    install : true,
    cpp_args : [ '-DSM_CORE_BUILD=1' ],
    include_directories : core_include,
    dependencies : core_dependencies
)

core = declare_dependency(
    link_with : libcore,
    include_directories : core_include,
    dependencies : core_dependencies
)
