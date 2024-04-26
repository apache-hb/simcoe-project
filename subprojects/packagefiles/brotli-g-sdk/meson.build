project('brotli-g', 'cpp',
    version : '1.1.0',
    default_options : [
        'cpp_std=c++17'
    ],
    license : 'LicenseRef-Proprietary',
    license_files : [ 'LICENSE.txt' ]
)

brotli_common = dependency('libbrotlicommon')
brotli_encode = dependency('libbrotlidec')
brotli_decode = dependency('libbrotlienc')

src = [
    'src/common/BrotligDataConditioner.cpp',
    'src/common/BrotligSwizzler.cpp',
    'src/common/BrotligUtils.cpp',
    'src/decoder/BrotligHuffmanTable.cpp',
    'src/decoder/PageDecoder.cpp',
    'src/encoder/BrotligHuffman.cpp',
    'src/encoder/PageEncoder.cpp',
    'src/BrotligDecoder.cpp',
    'src/BrotligEncoder.cpp',
    'src/DataStream.cpp',
]

inc = include_directories('inc', 'inc/common', 'inc/decoder', 'inc/encoder')

libbrotlig = library('brotlig', src,
    include_directories : inc,
    dependencies : [ brotli_common, brotli_encode, brotli_decode ]
)

brotlig_dep = declare_dependency(
    link_with : libbrotlig,
    include_directories : inc,
    dependencies : [ brotli_common, brotli_encode, brotli_decode ],
    extra_files : 'src/decoder/BrotliGCompute.hlsl'
)

install_data('src/decoder/BrotliGCompute.hlsl')

meson.override_dependency('brotlig', brotlig_dep)
