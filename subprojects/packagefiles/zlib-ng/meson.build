project('zlib-ng', 'c',
    version : '2.1.6',
    license : 'ZLIB',
    license_files : 'LICENSE.md',
    default_options : [
        'c_std=c11'
    ],
    meson_version : '>=1.1.0',
)

cc = meson.get_compiler('c')

buildtype = get_option('buildtype')
opt_ext = get_option('extensions')
opt_strategies = get_option('new_strategies')
opt_inflate_strict = get_option('inflate_strict')
opt_inflate_invalid_dist = get_option('inflate_invalid_dist')
opt_reduced_memory = get_option('reduced_mem')

c_args = [
    '-D_CRT_SECURE_NO_DEPRECATE',
    '-D_CRT_NONSTDC_NO_DEPRECATE'
]

if cc.get_id() != 'msvc'
    c_args += [ '-DHAVE_ATTRIBUTE_ALIGNED' ]
endif

if buildtype.contains('debug')
    c_args += [ '-DZLIB_DEBUG' ]
endif

includes = [ include_directories('.', 'arch/x86') ]

src = [
    # assume we always have xsave
    'arch/x86/x86_features.c',

    'adler32.c',
    'adler32_fold.c',
    'chunkset.c',
    'compare256.c',
    'compress.c',
    'cpu_features.c',
    'crc32_braid.c',
    'crc32_braid_comb.c',
    'crc32_fold.c',
    'deflate.c',
    'deflate_fast.c',
    'deflate_huff.c',
    'deflate_medium.c',
    'deflate_quick.c',
    'deflate_rle.c',
    'deflate_slow.c',
    'deflate_stored.c',
    'functable.c',
    'infback.c',
    'inflate.c',
    'inftrees.c',
    'insert_string.c',
    'insert_string_roll.c',
    'slide_hash.c',
    'trees.c',
    'uncompr.c',
    'zutil.c'
]

c_args += [
    '-DX86_FEATURES'
]

if opt_ext.contains('sse2')
    c_args += [ '-DX86_SSE2' ]
    src += [
        'arch/x86/chunkset_sse2.c',
        'arch/x86/compare256_sse2.c',
        'arch/x86/slide_hash_sse2.c'
    ]
endif

# TODO: figure out how to nicely figure out which extensions are available on the target
# if opt_ext.contains('ssse3')
#     c_args += [ '-DX86_SSSE3' ]
#     src += [
#         'arch/x86/adler32_ssse3.c',
#         'arch/x86/chunkset_ssse3.c'
#     ]
# endif

# if opt_ext.contains('sse42')
#     c_args += [ '-DX86_SSE42' ]
#     src += [
#         'arch/x86/adler32_sse42.c',
#         'arch/x86/insert_string_sse42.c'
#     ]
# endif

# if opt_ext.contains('avx2')
#     c_args += [ '-DX86_AVX2' ]
#     src += [
#         'arch/x86/slide_hash_avx2.c',
#         'arch/x86/chunkset_avx2.c',
#         'arch/x86/compare256_avx2.c',
#         'arch/x86/adler32_avx2.c'
#     ]
# endif

fs = import('fs')

configure_file(
    input : 'zconf-ng.h.in',
    output : 'zconf-ng.h',
    copy : true
)

configure_file(
    input : 'zlib_name_mangling.h.in',
    output : 'zlib_name_mangling-ng.h',
    configuration : {
        'ZLIB_SYMBOL_PREFIX': ''
    }
)

configure_file(
    input : 'zlib-ng.h.in',
    output : 'zlib-ng.h',
    copy : true
)

includes += [ include_directories('.') ]

deps = [ cc.find_library('m', required : false) ]

zlib_ng = library('zlib-ng', src,
    include_directories : includes,
    c_args : c_args,
    dependencies : deps
)

zlib_ng_dep = declare_dependency(
    include_directories : includes,
    link_with : zlib_ng,
    dependencies : deps
)
