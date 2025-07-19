project('simcoe', [ 'c', 'cpp' ],
    license : 'GPL-3.0-only',
    version : '1.1.0',
    default_options : [
        'warning_level=3',
        'werror=true',

        'cpp_std=c++latest,gnu++23',
        'default_library=static',
        'b_vscrt=mt',

        'cpp_rtti=false',
        'cpp_eh=default',

        # 'unity=subprojects',

        'licensedir=licenses',
        'wrap_mode=forcefallback',

        # configure freetype before harfbuzz to pick up configuration
        # 'freetype2:auto_features=disabled',
        # 'freetype2:zlib=disabled',
        # 'freetype2:png=disabled',
        # 'freetype2:harfbuzz=disabled',

        # harfbuzz has some logic when building with c++11 that cause build issues
        # 'harfbuzz:cpp_std=c++20',
        # 'harfbuzz:tests=disabled',
        # 'harfbuzz:freetype=enabled',
    ],
    meson_version : '>=1.3.0',
)

cpp = meson.get_compiler('cpp')
cc = meson.get_compiler('c')
is_windows = host_machine.system() == 'windows'

add_global_arguments('-D_ITERATOR_DEBUG_LEVEL=0', language : 'cpp')

add_global_arguments(cpp.get_supported_arguments('-Wno-assume', '-Wno-deprecated-literal-operator'), language : [ 'c', 'cpp' ])

if cpp.get_id() == 'clang-cl' and get_option('b_sanitize').contains('address')
    libs = [
        'C:/Program Files/LLVM/lib/clang/20/lib/windows/clang_rt.asan_dynamic_runtime_thunk-x86_64.lib',
        'C:/Program Files/LLVM/lib/clang/20/lib/windows/clang_rt.asan_dynamic-x86_64.lib',
    ]
    add_global_link_arguments(libs, language : [ 'c', 'cpp' ])
endif

if is_windows
    add_project_arguments('-DNOMINMAX', language : [ 'c', 'cpp' ])
endif

if get_option('tracy_enable')
    add_global_arguments('-DTRACY_ENABLE', language : [ 'c', 'cpp' ])
endif

fs = import('fs')
dxc_id = 'dxc-' + host_machine.system()
dxcompiler_id = 'dxcompiler-' + host_machine.system()

add_project_arguments(
    cc.get_supported_arguments(
        '/GS-', '/wd4127', '/wd4324',
        '/wd4200', '/wd4505',
        '-Wno-gnu-anonymous-struct', '-Wno-nested-anon-types',
        '-Wno-cast-function-type-mismatch'
    ),
    language : [ 'c', 'cpp' ]
)

build_type = get_option('buildtype')
is_debug = build_type.contains('debug')
is_release = build_type.contains('release')
library_type = get_option('default_library')

# funny things happen if something else pulls this in before us
dependency('libzstd')

# the perfect library
cthulhu = subproject('cthulhu')

rtsan_args = cpp.get_supported_arguments(
    '-Wfunction-effects',
    '-Wperf-effect-implies-noexcept',
)

add_project_arguments(rtsan_args, language : [ 'c', 'cpp' ])

args = cpp.get_supported_arguments(
    '-fstrict-aliasing',
    '-fno-keep-static-consts',
    '-fmerge-all-constants',
    '/Zc:threadSafeInit-',
    '/Zc:__cplusplus',
    # '/Zc:dllexportInlines-',

    # nicer messages
    '-fansi-escape-codes',

    # fmtlib forgot to mark a single argument as [[maybe_unused]]. pain
    '-Wno-unused-parameter',
)

cpp_args = cpp.get_supported_arguments(
    '/wd4251',
    '/wd4201',
    '/wd4244', # weird narrowing stuff i cant be bothered to figure out
    '/wd4702', # unreachable code in fmtlib
    '-fforce-emit-vtables',
    '-fno-threadsafe-statics',
    '-Wno-pedantic',
    '-Wno-missing-field-initializers'
)

if get_option('trace_time')
    args += cpp.get_supported_arguments('-ftime-trace')

    ninjatracing = find_program('scripts/ninjatracing/ninjatracing', native : true)

    custom_target('gather_time_trace',
        output : 'trace.json',
        capture : true,
        command : [ ninjatracing, meson.global_build_root() / '.ninja_log' ]
    )
endif

xclang_args = [
    '-funsafe-math-optimizations',
    '-mglobal-merge',
    '-ffast-math',
    '-fno-math-errno',
    '-fnew-infallible',
    '-fassume-nothrow-exception-dtor',
]

xclang_cpp_args = [
    '-fstrict-vtable-pointers',
    '-Wno-missing-field-initializers',

    # TODO: this could be UB in the future
    # note: if formatting is doing weird stuff check here
    '-Wno-nan-infinity-disabled',
]

foreach arg : xclang_args
    if cc.has_multi_arguments('-Xclang', arg)
        args += [ '-Xclang', arg ]
    endif
endforeach

foreach arg : xclang_cpp_args
    if cpp.has_multi_arguments('-Xclang', arg)
        cpp_args += [ '-Xclang', arg ]
    endif
endforeach

if get_option('b_lto')
    foreach arg : [ '-fwhole-program-vtables', '-fvirtual-function-elimination' ]
        if cpp.has_multi_arguments('-Xclang', arg)
            cpp_args += [ '-Xclang', arg ]
        endif
    endforeach
endif

add_project_arguments(cpp_args, language : 'cpp')
add_project_arguments(args, language : ['c', 'cpp'])

res_include = include_directories('data/win32')

version = meson.project_version()
parts = version.split('.')

# generate the project config header
simcoe_cdata = configuration_data()
simcoe_cdata.set('SMC_VERSION_PATCH', parts[2].to_int())
simcoe_cdata.set('SMC_VERSION_MINOR', parts[1].to_int())
simcoe_cdata.set('SMC_VERSION_MAJOR', parts[0].to_int())
simcoe_cdata.set_quoted('SMC_VERSION', version)
simcoe_cdata.set_quoted('SMC_SOURCE_DIR', meson.project_source_root().replace('\\', '\\\\'))
simcoe_cdata.set_quoted('SMC_ENGINE_NAME', get_option('engine_name'))

simcoe_cdata.set10('SMC_DEBUG', build_type.contains('debug'))
simcoe_cdata.set10('SMC_RELEASE', build_type.contains('release'))

simcoe_cdata.set10('SMC_BUILD_SHARED', library_type == 'shared')
simcoe_cdata.set10('SMC_BUILD_STATIC', library_type == 'static')

simcoe_config = configure_file(
    output : 'simcoe_config.h',
    configuration : simcoe_cdata
)

config_include = include_directories('.')

###
### engine modules
###

# we vendor fmtlib via a small shim header to hook FMT_THROW(x) into cthulhu's error handling
subdir('src/vendor/fmtlib')

subdir('src/common/base')
subdir('src/common/core')

# structured logging
subdir('src/common/logs/logs')

subdir('src/common/docker')

subdir('src/common/db')

subdir('src/common/logs/sinks')

subdir('src/common/config')
subdir('src/common/math')
subdir('src/common/archive')

subdir('src/common/input')
subdir('src/common/system')
subdir('src/common/threads')

subdir('src/common/net')

# vendor imgui here to have access to our own math library
subdir('src/vendor/imgui')
subdir('src/vendor/implot')
subdir('src/vendor/imguizmo')
subdir('src/vendor/imfilebrowser')
subdir('src/vendor/imnodes')
subdir('src/vendor/imgui_memory_editor')

subdir('src/game/account')

subdir('test/common/docker')

if host_machine.system() == 'windows'
    subdir('src/vendor/reflect')

    dxc = find_program(dxc_id, native : true)
    # msdf_atlas_gen = find_program('msdf-atlas-gen', native : true)
    # compressonator = find_program('compressonatorcli', native : true)
    # fastgltf = dependency('fastgltf')

    d3d12 = dependency('d3d12')
    d3dx12 = dependency('directx-headers')
    tracy = dependency('tracy')

    # subdir('src/common/world')

    subdir('src/vendor/d3d12ma')

    subdir('src/common/render')
    subdir('src/common/graph')
    subdir('src/common/draw')
    # subdir('src/common/game')

    ###
    ### plugins
    ###

    subdir('src/plugins/ryzenmaster')

    ###
    ### game modules
    ###

    # subdir('src/game/thunderflash')
endif

###
### top level modules
###

subdir('src/common/launch')

###
### plugins
###

subdir('src/plugins/orarpc')

if host_machine.system() == 'windows'
    subdir('src/plugins/contextmenu')

    # windows specific dependencies
    # d3d12 = subproject('d3d12')
    # warp = subproject('d3dwarp')
    # dxcompiler = subproject(dxc_id)
    # winpixruntime = subproject('winpixruntime')
    # dstorage = subproject('dstorage')
    windows = import('windows')

    magick = find_program('magick', native : true)
    convert_utf16 = find_program('scripts/convert_utf16.py', native : true)

    default_ico = custom_target('default_icon',
        input : [ 'data/assets/icon/icon_circle.png' ],
        command : [ magick, '@INPUT@', '-define', 'icon:auto-resize=256,128,48,32,16', '@OUTPUT@' ],
        output : [ 'icon_default.ico' ]
    )

    rc_cdata = configuration_data()
    rc_cdata.merge_from(simcoe_cdata)
    rc_cdata.set('DEFAULT_ICON', default_ico.full_path().replace('\\', '\\\\'))
    rc_cdata.set('MAJOR', parts[0].to_int())
    rc_cdata.set('MINOR', parts[1].to_int())
    rc_cdata.set('PATCH', parts[2].to_int())
    rc_cdata.set('BUILD', 99)
    rc_cdata.set('COMPANY', 'Temptation Games')
    rc_cdata.set('PRODUCT', 'Priority Zero')
    rc_cdata.set('DESCRIPTION', 'Priority Zero')

    rc_cdata.set('COMPANY_ID', 'TemptationGames')
    rc_cdata.set('PRODUCT_ID', 'PriorityZero')

    rc = configure_file(
        input : 'data/win32/client.rc.in',
        output : 'client.temp.rc',
        configuration : rc_cdata
    )

    rc_utf16 = custom_target('client.rc',
        input : [ rc ],
        output : [ 'client.rc' ],
        command : [ convert_utf16, '@INPUT@', '@OUTPUT@' ]
    )

    # icky, rc actually depends on default_ico, but meson doesn't support
    # custom_target dependencies on generated files
    res = windows.compile_resources(rc,
        depends : [ default_ico ],
        include_directories : res_include
    )

    # bundle_cdata = configuration_data()
    # bundle_cdata.set('DXC_EXE_PATH', dxc.full_path())
    # bundle_cdata.set('MSDF_EXE_PATH', msdf_atlas_gen.full_path())
    # bundle_cdata.set('COMPRESSOR_EXE_PATH', compressonator.full_path())
    # bundle_cdata.set('AGILITY_REDIST_PATH', d3d12.get_variable('bindir'))
    # bundle_cdata.set('DXCOMPILER_REDIST_PATH', dxcompiler.get_variable('bindir'))
    # bundle_cdata.set('WARP_REDIST_PATH', warp.get_variable('bindir'))
    # bundle_cdata.set('WINPIXRUNTIME_REDIST_PATH', winpixruntime.get_variable('bindir'))
    # bundle_cdata.set('DSTORAGE_REDIST_PATH', dstorage.get_variable('bindir'))

    # bundle_config = configure_file(
    #     input : 'data/config/tools.in.ini',
    #     output : 'tools.ini',
    #     configuration : bundle_cdata
    # )

    # make_bundle = find_program('scripts/make_bundle.py')

    # bundle_cmd = [
    #     make_bundle,
    #     '--desc', '@INPUT@',
    #     '--config', bundle_config,
    #     '--dir', '@SOURCE_ROOT@/data/assets',
    #     '--outdir', '@OUTDIR@',
    #     '--bundle', '@OUTPUT@',
    #     '--depfile', '@DEPFILE@',
    #     '--root', meson.current_source_dir()
    # ]

    # if is_debug
    #     bundle_cmd += [ '--debug' ]
    # endif

    # game_bundle = custom_target('game_bundle',
    #     input : [ 'data/assets/bundle.json' ],
    #     output : 'bundle.tar',
    #     # install : true,
    #     # install_dir : 'bin',
    #     depfile : 'bundle.d',
    #     command : bundle_cmd
    # )

    # link_args = [
    #     '/DELAYLOAD:dxcompiler.dll',
    #     '/DELAYLOAD:dxil.dll',
    #     '/DELAYLOAD:WinPixEventRuntime.dll',
    #     '/DELAYLOAD:dstoragecore.dll',
    #     '/DELAYLOAD:dstorage.dll',
    # ]

    manifest = meson.current_source_dir() / 'data/win32/client.exe.manifest'

    subdir('src/projects/thunderflash/client')

    subdir('src/projects/thunderflash/editor')
endif

subdir('src/server')
