project('riegeli', 'cpp',
    default_options : [
        'cpp_std=c++17',
    ],
    license : 'Apache-2.0',
    license_files : 'LICENSE',
    meson_version : '>=1.1.0',
)

src = [
    'riegeli/base/assert.cc'
]

deps = [
    dependency('absl_base')
]

libriegeli = library('riegeli', src,
    include_directories : '.',
    override_options : [ 'unity=off' ],
    dependencies : deps,
)

riegeli_dep = declare_dependency(
    link_with : libriegeli,
    include_directories : '.',
    dependencies : deps,
)

meson.override_dependency('riegeli', riegeli_dep)
