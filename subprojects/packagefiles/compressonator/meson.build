project('compressonator', 'cpp',
    default_options : [
        'cpp_std=c++17,c++14,c++11',
        'warning_level=3',
        'werror=true'
    ],
    version : '4.5.0',
    license : 'LicenseRef-Proprietary',
    license_files : [ 'license/license_file' ]
)

cpp = meson.get_compiler('cpp')

core_args = [
    '-D_CMP_CPP17_=1'
]

brotlig = dependency('brotlig', required : get_option('brotlig'))

if get_option('buildtype').contains('debug')
    core_args += [ '-D_DEBUG' ]
endif

###
### core libraries
###

src = [
    'cmp_core/shaders/bc1_encode_kernel.cpp',
    'cmp_core/shaders/bc2_encode_kernel.cpp',
    'cmp_core/shaders/bc3_encode_kernel.cpp',
    'cmp_core/shaders/bc4_encode_kernel.cpp',
    'cmp_core/shaders/bc5_encode_kernel.cpp',
    'cmp_core/shaders/bc6_encode_kernel.cpp',
    'cmp_core/shaders/bc7_encode_kernel.cpp',

    'cmp_core/source/cmp_core.cpp',

    'applications/_libs/cmp_math/cpu_extensions.cpp',
    'applications/_libs/cmp_math/cmp_math_common.cpp',
]

inc = include_directories('cmp_core/shaders', 'cmp_core/source', 'applications/_libs/cmp_math')

if brotlig.found()
    src += [ 'cmp_core/shaders/brlg_encode_kernel.cpp' ]
endif

libcore = library('cmp_core', src,
    include_directories : inc,
    cpp_args : core_args,
    vs_module_defs : 'cmp_core/cmp_core.def'
)

core_dep = declare_dependency(
    link_with : libcore,
    compile_args : core_args,
    include_directories : inc,
)

libcore_sse = library('cmp_core_sse', 'cmp_core/source/core_simd_sse.cpp',
    dependencies : core_dep
)

core_sse_dep = declare_dependency(
    link_with : libcore_sse,
    dependencies : core_dep
)

libcore_avx = library('cmp_core_avx', 'cmp_core/source/core_simd_avx.cpp',
    dependencies : core_dep
)

core_avx_dep = declare_dependency(
    link_with : libcore_avx,
    dependencies : core_dep
)

libcore_avx512 = library('cmp_core_avx512', 'cmp_core/source/core_simd_avx512.cpp',
    dependencies : core_dep
)

core_avx512_dep = declare_dependency(
    link_with : libcore_avx512,
    dependencies : core_dep
)

###
### framework libraries
###

framework_args = [
    '-DCMP_USE_XMMINTRIN=1'
]

framework_src = [
    # HALF_SRC
    'cmp_framework/common/half/half.cpp',

    # DDS_SRC
    'applications/_plugins/cimage/dds/dds_dx10.cpp',
    'applications/_plugins/cimage/dds/dds_file.cpp',
    'applications/_plugins/cimage/dds/dds_helpers.cpp',
    'applications/_plugins/cimage/dds/dds.cpp',

    # HPC_SRC
    'applications/_plugins/ccmp_encode/hpc/ccpu_hpc.cpp',
    'applications/_plugins/ccmp_encode/hpc/cmp_hpc.cpp',
    'applications/_plugins/ccmp_encode/hpc/compute_cpu_hpc.cpp',

    # CMP_FRAMEWORK_SRC
    'cmp_framework/compute_base.cpp',

    # CMP_FRAMEWORK_COMMON_SRC
    'cmp_framework/common/cmp_boxfilter.cpp',
    'cmp_framework/common/cmp_mips.cpp',
    'cmp_framework/common/hdr_encode.cpp',

    # CMP_Math
    'applications/_libs/cmp_math/cmp_math_common.cpp',
    'applications/_libs/cmp_math/cpu_extensions.cpp',

    # CMP_COMMON
    'applications/_plugins/common/atiformats.cpp',
    'applications/_plugins/common/pluginmanager.cpp',
    'applications/_plugins/common/cpu_timing.cpp',
    'applications/_plugins/common/tc_plugininternal.cpp',
    'applications/_plugins/common/utilfuncs.cpp',
    'applications/_plugins/common/cmp_fileio.cpp',
    'applications/_plugins/common/format_conversion.cpp',
    'applications/_plugins/common/codec_common.cpp',
    'applications/_plugins/common/texture_utils.cpp',

    # CMP_SDK Lossy Compression
    'applications/_plugins/ccmp_sdk/bcn.cpp',
    'applications/_plugins/ccmp_sdk/bc1/bc1.cpp',
    'applications/_plugins/ccmp_sdk/bc2/bc2.cpp',
    'applications/_plugins/ccmp_sdk/bc3/bc3.cpp',
    'applications/_plugins/ccmp_sdk/bc4/bc4.cpp',
    'applications/_plugins/ccmp_sdk/bc5/bc5.cpp',
    'applications/_plugins/ccmp_sdk/bc6/bc6h.cpp',
    'applications/_plugins/ccmp_sdk/bc7/bc7.cpp',
]

framework_deps = [ dependency('stb_image') ]

if brotlig.found()
    framework_src += [
        'applications/_plugins/ccmp_sdk/brlg/codec_brlg.cpp',
        'cmp_compressonatorlib/brotlig/brlg_sdk_wrapper.cpp',
        'cmp_compressonatorlib/brotlig/BrotligCompute.cpp',
    ]

    framework_deps += [
        brotlig,
        dependency('d3d12'),
        dependency('dxcompiler')
    ]
endif

libframework = library('cmp_framework', framework_src,
    include_directories : inc,
    cpp_args : framework_args,
    dependencies : framework_deps,
    vs_module_defs : 'cmp_framework/cmp_framework.def'
)

framework_dep = declare_dependency(
    link_with : libframework,
    compile_args : framework_args,
    include_directories : inc,
    dependencies : framework_deps
)

meson.override_dependency('cmp_core', core_dep)
meson.override_dependency('cmp_framework', framework_dep)
