bundle_include = include_directories('.', 'include')

src = [
    'src/bundle.cpp',
    reflect.process('data/bundle.ct')
]

deps = [ core, logs ]

libbundle = library('bundle', src,
    cpp_args : [ '-DSM_BUNDLE_BUILD=1' ],
    include_directories : [ bundle_include, reflect_include ],
    dependencies : deps
)

bundle = declare_dependency(
    link_with : libbundle,
    include_directories : [ bundle_include, libbundle.private_dir_include() ],
    dependencies : deps
)
