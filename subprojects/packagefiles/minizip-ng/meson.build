project('minizip-ng', 'c',
    version : '4.0.4',
    license : 'ZLIB',
    license_files : [ 'LICENSE' ]
)

src = [
    'mz_crypt.c',
    'mz_os.c',
    'mz_strm.c',
    'mz_strm_buf.c',
    'mz_strm_mem.c',
    'mz_strm_split.c',
    'mz_zip.c',
    'mz_zip_rw.c'
]

includes = include_directories('.')

args = [
    '-DMZ_ZIP_NO_COMPRESSION',
    '-DMZ_ZIP_NO_ENCRYPTION',
    '-D_CRT_SECURE_NO_DEPRECATE',
    '-D_CRT_NONSTDC_NO_DEPRECATE'
]

zlib_ng = dependency('zlib-ng')
opt_comps = get_option('compressors')
opt_features = get_option('features')

minizip_ng = library('minizip-ng', src,
    include_directories : includes,
    dependencies : zlib_ng,
    c_args : args
)

minizip_ng_dep = declare_dependency(
    include_directories : includes,
    dependencies : zlib_ng,
    link_with : minizip_ng
)
