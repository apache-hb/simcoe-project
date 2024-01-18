project('simcoe', [ 'c', 'cpp' ],
    version : '0.0.1',
    default_options : [
        'cpp_std=c++20',

        'cpp_rtti=false',
        'cpp_eh=none',

        'warning_level=3',
        'werror=true',

        'default_library=static'
    ]
)

cpp = meson.get_compiler('cpp')

cthulhu = subproject('cthulhu')

reflect_include = cthulhu.get_variable('reflect_include')
reflect = cthulhu.get_variable('reflect')

add_project_arguments('-D_HAS_EXCEPTIONS=0', language : 'cpp')
add_project_arguments('/wd4251', language : 'cpp')

subdir('src/common/core')
subdir('src/common/math')
subdir('src/vendor/imgui')
subdir('src/common/service')
subdir('src/common/system')
subdir('src/common/rhi')

client = executable('client', 'src/client/src/main.cpp',
    dependencies : [
        core, service,
        cthulhu.get_variable('backtrace'),
        cthulhu.get_variable('format'),
        cthulhu.get_variable('io')
    ]
)

# apply app manifest to client executable
manifest = find_program('scripts/manifest.py')
custom_target('client_manifest',
    build_by_default : true,
    depends : client,
    input : [ 'data/client.exe.manifest', client ],
    output : [ 'client_final.exe' ],
    command : [ manifest, '@INPUT0@', '@INPUT1@', '@OUTPUT@' ],
)
