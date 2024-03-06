project('stb', 'c')

cc = meson.get_compiler('c')


add_project_arguments('-D_CRT_SECURE_NO_WARNINGS=1', language : 'c')

src = [
    'src/stb_image.c',
    'src/stb_image_write.c',
    'src/stb_rectpack.c',
    'src/stb_perlin.c'
]

args = [
    '-D_CRT_SECURE_NO_WARNINGS=1',
    '-DSTBI_NO_STDIO=1',
    '-DSTBI_WRITE_NO_STDIO=1'
]

args = cc.get_supported_arguments('/wd4244')

libstb = library('stb', src,
    include_directories : [ 'include' ],
    c_args : [ '/wd4244' ]
)

stb = declare_dependency(
    include_directories : [ 'include' ],
    link_with : [ libstb ]
)
