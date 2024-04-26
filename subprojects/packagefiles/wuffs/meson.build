project('wuffs', 'c',
    license : 'MIT AND Apache-2.0',
    license_files : [ 'LICENSE', 'LICENSE-MIT', 'LICENSE-APACHE' ]
)

config = {
    'WUFFS_IMPLEMENTATION': true,
    'WUFFS_CONFIG__MODULES': true,

    'WUFFS_CONFIG__MODULE__BASE': true,
    'WUFFS_CONFIG__MODULE__BASE__MAGIC': get_option('magic'),
    'WUFFS_CONFIG__MODULE__BASE__INTCONV': get_option('intconv'),
    'WUFFS_CONFIG__MODULE__BASE__FLOATCONV': get_option('floatconv'),
    'WUFFS_CONFIG__MODULE__BASE__PIXCONV': get_option('pixconv'),

    'WUFFS_CONFIG__MODULE__ADLER32': get_option('adler32'),
    'WUFFS_CONFIG__MODULE__CRC32': get_option('crc32'),

    'WUFFS_CONFIG__MODULE__BMP': get_option('bmp'),
    'WUFFS_CONFIG__MODULE__GIF': get_option('gif'),
    'WUFFS_CONFIG__MODULE__NIE': get_option('nie'),
    'WUFFS_CONFIG__MODULE__PNG': get_option('png'),
    'WUFFS_CONFIG__MODULE__TGA': get_option('tga'),
    'WUFFS_CONFIG__MODULE__WBMP': get_option('wbmp'),

    'WUFFS_CONFIG__MODULE__BZIP2': get_option('bzip2'),
    'WUFFS_CONFIG__MODULE__GZIP': get_option('gzip'),
    'WUFFS_CONFIG__MODULE__ZLIB': get_option('zlib'),
    'WUFFS_CONFIG__MODULE__DEFLATE': get_option('deflate'),
    'WUFFS_CONFIG__MODULE__LZW': get_option('lzw'),

    'WUFFS_CONFIG__MODULE__JSON': get_option('json'),
    'WUFFS_CONFIG__MODULE__CBOR': get_option('cbor'),

    'WUFFS_CONFIG__MODULE__AUX__BASE': get_option('aux-base'),
    'WUFFS_CONFIG__MODULE__AUX__CBOR': get_option('aux-cbor'),
    'WUFFS_CONFIG__MODULE__AUX__IMAGE': get_option('aux-image'),
    'WUFFS_CONFIG__MODULE__AUX__JSON': get_option('aux-json'),
}

defines = [ ]

foreach define, enabled : config
    if enabled
        defines += [ '-D' + define ]
    endif
endforeach

fs = import('fs')

source = fs.copyfile('release/c/wuffs-v0.3.c', 'wuffs.c')
header = fs.copyfile('release/c/wuffs-v0.3.c', 'wuffs.h')

libwuffs = static_library('wuffs', source, header,
    c_args : defines
)

wuffs_dep = declare_dependency(
    sources : header,
    link_with : libwuffs,
    compile_args : defines
)
