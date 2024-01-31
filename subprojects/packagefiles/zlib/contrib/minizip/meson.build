src = [
    'ioapi.c',
    'iowin32.c',
    'unzip.c',
    'zip.c',
]

minizip_include = include_directories('.')

minizip = library('minizip', src,
    include_directories : minizip_include,
    dependencies : [ zlib_dep ]
)

minizip_dep = declare_dependency(
    include_directories : minizip_include,
    link_with : minizip,
    dependencies : [ zlib_dep ]
)
