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

opt_zlib = get_option('zlib')