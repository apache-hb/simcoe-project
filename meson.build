project('simcoe', [ 'c', 'cpp' ],
    version : '1.0.1',
    default_options : [
        'cpp_std=c++20',

        'cpp_rtti=false',
        'cpp_eh=none',

        'warning_level=3',
        'werror=true',

        'default_library=static',

        'cthulhu:default_library=static',
        'cthulhu:auto_features=disabled',
        'cthulhu:tool_reflect=enabled'
    ]
)

cpp = meson.get_compiler('cpp')

cthulhu = subproject('cthulhu')
windows = import('windows')

magick = find_program('magick')
convert_utf16 = find_program('scripts/convert_utf16.py')

reflect_include = cthulhu.get_variable('reflect_include')
reflect = cthulhu.get_variable('reflect')

args = cpp.get_supported_arguments(
    '/GS-',
    '-fstrict-aliasing',
    '-fno-threadsafe-statics',
    '-fno-keep-static-consts',
    '-fmerge-all-constants')

add_project_arguments(args, language : 'cpp')
add_project_arguments('-D_HAS_EXCEPTIONS=0', language : 'cpp')
add_project_arguments('/wd4251', language : 'cpp')

res_include = include_directories('data/win32')

# generate the project config header
version = meson.project_version()
parts = version.split('.')
build_type = get_option('buildtype')
library_type = get_option('default_library')

package_type = get_option('package_type')

simcoe_cdata = configuration_data()
simcoe_cdata.set('SMC_VERSION_PATCH', parts[2].to_int())
simcoe_cdata.set('SMC_VERSION_MINOR', parts[1].to_int())
simcoe_cdata.set('SMC_VERSION_MAJOR', parts[0].to_int())
simcoe_cdata.set_quoted('SMC_VERSION', version)

simcoe_cdata.set_quoted('SMC_PACKAGE_TYPE', package_type)
simcoe_cdata.set10('SMC_BUILD_DEVEL', package_type == 'devel')
simcoe_cdata.set10('SMC_BUILD_PRIVATE', package_type == 'private')
simcoe_cdata.set10('SMC_BUILD_RELEASE', package_type == 'release')

simcoe_cdata.set10('SMC_DEBUG', build_type.contains('debug'))
simcoe_cdata.set10('SMC_RELEASE', build_type.contains('release'))

simcoe_cdata.set10('SMC_BUILD_SHARED', library_type == 'shared')
simcoe_cdata.set10('SMC_BUILD_STATIC', library_type == 'static')

simcoe_config = configure_file(
    output : 'simcoe_config.h',
    configuration : simcoe_cdata
)

config_include = include_directories('.')

subdir('src/common/core')
subdir('src/common/math')
subdir('src/vendor/imgui')
subdir('src/common/service')
subdir('src/common/system')
subdir('src/common/rhi')

default_ico = custom_target('default_icon',
    input : [ 'data/assets/icon/icon_circle.png' ],
    command : [ magick, '@INPUT@', '-define', 'icon:auto-resize=256,128,48,32,16', '@OUTPUT@' ],
    output : [ 'icon_default.ico' ]
)

rc_cdata = configuration_data()
rc_cdata.merge_from(simcoe_cdata)
rc_cdata.set('DEFAULT_ICON', default_ico.full_path())
rc_cdata.set('MAJOR', parts[0].to_int())
rc_cdata.set('MINOR', parts[1].to_int())
rc_cdata.set('PATCH', parts[2].to_int())
rc_cdata.set('BUILD', 99)

rc_temp = configure_file(
    input : 'data/win32/client.in.rc',
    output : 'client.temp.rc',
    configuration : rc_cdata
)

rc = custom_target('client.rc',
    input : [ rc_temp ],
    output : [ 'client.rc' ],
    command : [ convert_utf16, '@INPUT@', '@OUTPUT@' ]
)

# icky, rc actually depends on default_ico, but meson doesn't support
# custom_target dependencies on generated files
res = windows.compile_resources(rc,
    depends : [ default_ico ],
    include_directories : res_include
)

manifest = meson.current_source_dir() / 'data/win32/client.exe.manifest'

client = executable('client', 'src/client/src/main.cpp',
    install : true,
    link_depends : [ manifest, res ],
    link_args : [
        # embed the resource file
        res[0].full_path(),

        # embed the manifest file
        '/manifest:embed', '/manifestinput:' + manifest
    ],
    dependencies : [
        core, service, system,
        cthulhu.get_variable('backtrace'),
        cthulhu.get_variable('format_core'),
        cthulhu.get_variable('io')
    ]
)
