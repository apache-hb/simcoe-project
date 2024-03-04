archive_include = include_directories('.', 'include')

src = [
    'src/archive.cpp',
    'src/io.cpp',
    'src/record.cpp',
    'src/bundle.cpp',

    reflect.process('data/format.ct'),
    reflect.process('data/io.ct'),
    reflect.process('data/record.ct')
]

deps = [
    core, logs, math, stb,
    cthulhu.get_variable('fs'),
    cthulhu.get_variable('tar')
]

libarchive = library('archive', src,
    cpp_args : [ '-DSM_BUNDLE_BUILD=1' ],
    include_directories : [ archive_include, reflect_include ],
    dependencies : deps + [ artery ]
)

archive = declare_dependency(
    link_with : libarchive,
    include_directories : [ archive_include, libarchive.private_dir_include() ],
    dependencies : deps
)
