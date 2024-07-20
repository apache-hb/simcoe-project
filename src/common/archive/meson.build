archive_include = include_directories('.', 'include')

src = [
    'src/io.cpp',
    'src/record.cpp',
    'src/bundle.cpp',
    'src/image.cpp',
    'src/fs.cpp',
    'src/connection.cpp',

    reflect.process('data/format.ct'),
    reflect.process('data/io.ct'),
    reflect.process('data/record.ct')
]

deps = [
    core, logs, math,
    cthulhu.get_variable('io')
]

libarchive = library('archive', src,
    cpp_args : [ '-DSM_BUNDLE_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ archive_include, reflect_include ],
    dependencies : [
        deps,
        dependency('sqlite3'),
        dependency('stb_image'),
        dependency('liblz4'),
        dependency('libarchive'),
        dependency('artery-font')
    ]
)

archive = declare_dependency(
    link_with : libarchive,
    include_directories : [ archive_include, libarchive.private_dir_include() ],
    dependencies : deps
)
