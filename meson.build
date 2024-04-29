project('simcoe', [ 'c', 'cpp' ],
    # UNLICENSED does not mean public domain, this is my code do not use it
    license : 'UNLICENSED',
    version : '1.1.0',
    default_options : [
        'cpp_std=c++latest',

        'cpp_rtti=false',
        'cpp_eh=none',

        'warning_level=3',
        'werror=true',

        'unity=subprojects',
        'default_library=static',
        'b_vscrt=static_from_buildtype',

        # prevent system dxc from being used
        'force_fallback_for=dxc', # TODO: this doesnt seem to actually work. meson bug?

        'tracy:no_frame_image=true',
        'tracy:callstack=true',
        'tracy:only_localhost=true',

        'jolt:default_library=static',

        # nobody builds with clean warnings, so disable them per subproject
        'fmt:warning_level=0',
        # 'harfbuzz:warning_level=0',
        # 'freetype2:warning_level=0',
        'simdjson:warning_level=0',
        'fastgltf:warning_level=0',
        'tracy:warning_level=0',
        'lz4:warning_level=0',
        'libarchive:warning_level=0',
        'google-brotli:warning_level=0',
        'brotli_g_sdk:warning_level=0',

        # disable everything in cthulhu by default
        'cthulhu:auto_features=disabled',

        # configure fastgltf
        'fastgltf:custom_small_vector=true',

        'flecs:default_library=static',

        'libarchive:progs=[]',

        # configure freetype before harfbuzz to pick up configuration
        # 'freetype2:auto_features=disabled',
        # 'freetype2:zlib=disabled',
        # 'freetype2:png=disabled',
        # 'freetype2:harfbuzz=disabled',

        # harfbuzz has some logic when building with c++11 that cause build issues
        # 'harfbuzz:cpp_std=c++20',
        # 'harfbuzz:tests=disabled',
        # 'harfbuzz:freetype=enabled',
    ]
)

cpp = meson.get_compiler('cpp')
cc = meson.get_compiler('c')

add_project_arguments('-DNOMINMAX', language : [ 'c', 'cpp' ])
add_global_arguments(cc.get_supported_arguments('/GS-', '/wd4127', '/wd4324', '/wd4200', '/wd4505'), language : [ 'c', 'cpp' ])
add_global_arguments('-D_HAS_EXCEPTIONS=0', language : 'cpp')

build_type = get_option('buildtype')
is_debug = build_type.contains('debug')
library_type = get_option('default_library')

package_type = get_option('package_type')
trace_time = get_option('trace_time')
tracy_enable = get_option('tracy_enable')

if tracy_enable
    add_global_arguments('-DTRACY_ENABLE', language : [ 'c', 'cpp' ])
endif

is_devel = package_type == 'devel'
is_private = package_type == 'private'
is_release = package_type == 'release'

pix_enable = get_option('pix_enable').disable_auto_if(not is_devel)
warp_enable = get_option('warp_enable').disable_auto_if(is_release)

# the perfect library
cthulhu = subproject('cthulhu')

dxc = find_program('dxc', version : '>=1.8')
msdf_atlas_gen = find_program('msdf-atlas-gen')
compressonator = find_program('compressonatorcli')
artery = dependency('artery-font')
fastgltf = dependency('fastgltf')

dxgi = cpp.find_library('dxgi')
dxguid = cpp.find_library('dxguid')

d3d12 = dependency('d3d12')
dstorage = dependency('dstorage')
pix = dependency('winpixruntime')
d3dx12 = dependency('directx-headers')
tracy = dependency('tracy')

magick = find_program('magick')
convert_utf16 = find_program('scripts/convert_utf16.py')

args = cpp.get_supported_arguments(
    '-fstrict-aliasing',
    '-fno-threadsafe-statics',
    '-fno-keep-static-consts',
    '-fmerge-all-constants',
    '/Zc:threadSafeInit-',
    '/Zc:__cplusplus',
    '/Zc:dllexportInlines-',

    # nicer messages
    '-fansi-escape-codes',

    # fmtlib forgot to mark a single argument as [[maybe_unused]]. pain
    '-Wno-unused-parameter'
)

cpp_args = cpp.get_supported_arguments(
    '/wd4251',
    '/wd4201',
    '/wd4100',
    '/wd4244', # weird narrowing stuff i cant be bothered to figure out
    '/wd4702', # unreachable code in fmtlib
    '-fforce-emit-vtables'
)

if trace_time.enabled()
    args += cpp.get_supported_arguments('-ftime-trace')

    ninjatracing = find_program('scripts/ninjatracing/ninjatracing')

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
    '-fno-math-errno'
]

xclang_cpp_args = [
    '-fstrict-vtable-pointers',
    '-Wno-missing-field-initializers',

    # TODO: this could be UB in the future
    # note: if formatting is doing weird stuff check here
    '-Wno-nan-infinity-disabled'
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

simcoe_cdata.set_quoted('SMC_PACKAGE_TYPE', package_type)
simcoe_cdata.set10('SMC_BUILD_DEVEL', package_type == 'devel')
simcoe_cdata.set10('SMC_BUILD_PRIVATE', package_type == 'private')
simcoe_cdata.set10('SMC_BUILD_RELEASE', package_type == 'release')

simcoe_cdata.set10('SMC_DEBUG', build_type.contains('debug'))
simcoe_cdata.set10('SMC_RELEASE', build_type.contains('release'))

simcoe_cdata.set10('SMC_BUILD_SHARED', library_type == 'shared')
simcoe_cdata.set10('SMC_BUILD_STATIC', library_type == 'static')

simcoe_cdata.set10('SMC_PIX_ENABLE', pix_enable.allowed())
simcoe_cdata.set10('SMC_WARP_ENABLE', warp_enable.allowed())

simcoe_config = configure_file(
    output : 'simcoe_config.h',
    configuration : simcoe_cdata
)

config_include = include_directories('.')

# we vendor fmtlib via a small shim header to hook FMT_THROW(x) into cthulhu's error handling
subdir('src/vendor/fmtlib')

# reflection tool
# subdir('src/meta')

# extra_args must contain the name of the target, idk how to get it otherwise
# meta_tool = generator(find_program('scripts/meta_tool.py'),
#     output : [ '@BASENAME@.meta.h', '@BASENAME@.meta.cpp' ],
#     depends : meta_tooling,
#     arguments : [
#         '--builddir', meson.global_build_root(),
#         '--metatool', meta_tooling.full_path(),
#         '--reflect', '@INPUT@',
#         '--header', '@OUTPUT0@',
#         '--source', '@OUTPUT1@',
#         '--target', '@EXTRA_ARGS@'
#     ]
# )

subdir('src/vendor/reflect')

subdir('src/common/core')
# also hook asserts into d3d12ma
subdir('src/vendor/d3d12ma')

# patch dds_texture_loader to use d3d12ma
# subdir('src/vendor/dds_texture_loader')

subdir('src/common/math')
subdir('src/common/logs')
subdir('src/common/config')
subdir('src/common/archive')
subdir('src/common/input')
subdir('src/common/system')
subdir('src/common/world')
subdir('src/common/threads')

subdir('src/common/render')
subdir('src/common/draw')

# vendor imgui here to have access to our own math library
subdir('src/vendor/imgui')
subdir('src/vendor/implot')
subdir('src/vendor/imguizmo')
subdir('src/vendor/imfilebrowser')
subdir('src/vendor/imnodes')
subdir('src/vendor/imgui_memory_editor')

subdir('src/common/game')

# windows specific dependencies
agility = subproject('d3d12')
warp = subproject('d3dwarp')
dxcompiler = subproject('dxc')
winpixruntime = subproject('winpixruntime')
dstorage = subproject('dstorage')
windows = import('windows')

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

rc = configure_file(
    input : 'data/win32/client.in.rc',
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

bundle_cdata = configuration_data()
bundle_cdata.set('DXC_EXE_PATH', dxc.full_path())
bundle_cdata.set('MSDF_EXE_PATH', msdf_atlas_gen.full_path())
bundle_cdata.set('COMPRESSOR_EXE_PATH', compressonator.full_path())
bundle_cdata.set('AGILITY_REDIST_PATH', agility.get_variable('bindir'))
bundle_cdata.set('DXCOMPILER_REDIST_PATH', dxcompiler.get_variable('bindir'))
bundle_cdata.set('WARP_REDIST_PATH', warp.get_variable('bindir'))
bundle_cdata.set('WINPIXRUNTIME_REDIST_PATH', winpixruntime.get_variable('bindir'))
bundle_cdata.set('DSTORAGE_REDIST_PATH', dstorage.get_variable('bindir'))

bundle_config = configure_file(
    input : 'data/config/tools.in.ini',
    output : 'tools.ini',
    configuration : bundle_cdata
)

make_bundle = find_program('scripts/make_bundle.py')
game_bundle = custom_target('game_bundle',
    input : [ 'data/assets/bundle.json' ],
    output : 'bundle.tar',
    install : true,
    install_dir : 'bin',
    depfile : 'bundle.d',
    command : [
        make_bundle,
        '--desc', '@INPUT@',
        '--config', bundle_config,
        is_debug ? '--debug' : '',
        '--dir', '@SOURCE_ROOT@/data/assets',
        '--outdir', '@OUTDIR@',
        '--bundle', '@OUTPUT@',
        '--depfile', '@DEPFILE@',
        '--root', meson.current_source_dir()
    ]
)

win_subsystem = not is_devel ? 'windows' : 'console'

link_args = [
    '/DELAYLOAD:dxcompiler.dll',
    '/DELAYLOAD:dxil.dll',
    '/DELAYLOAD:WinPixEventRuntime.dll',
    '/DELAYLOAD:dstoragecore.dll',
    '/DELAYLOAD:dstorage.dll',
]

subdir('src/client')

manifest = meson.current_source_dir() / 'data/win32/client.exe.manifest'

client = executable('client', src, game_bundle,
    cpp_pch : pch,
    install : true,
    export_dynamic : true,
    win_subsystem : win_subsystem,
    link_depends : [ manifest, res ],
    link_args : link_args + [
        # embed the resource file
        res[0].full_path(),

        # embed the manifest file
        '/manifest:embed', '/manifestinput:' + manifest,
    ],
    dependencies : deps
)

subdir('src/editor')

editor = executable('editor', src, game_bundle,
    cpp_pch : pch,
    include_directories : inc,
    install : true,
    export_dynamic : true,
    win_subsystem : win_subsystem,
    link_depends : [ manifest, res ],
    link_args : link_args,
    dependencies : deps
)
