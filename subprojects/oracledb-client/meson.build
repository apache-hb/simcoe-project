project('oradb-oci', 'c',
    version : '21.0.0'
)

fs = import('fs')
cc = meson.get_compiler('c')

oci_path = get_option('home') / 'oci'

if not fs.is_dir(oci_path)
    error('Oracle Client not found at `@0@`'.format(oci_path))
endif

oci_include = include_directories(oci_path / 'include')
oci_lib = cc.find_library('oci', dirs : [ oci_path / 'lib' / 'msvc' ])

oci = declare_dependency(
    include_directories : [ oci_include ],
    dependencies : [ oci_lib ]
)

meson.override_dependency('oci', oci)
