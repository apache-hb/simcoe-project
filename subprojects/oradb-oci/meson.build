project('oradb-oci', 'c',
    version : '19.0.0'
)

fs = import('fs')
cc = meson.get_compiler('c')

base = get_option('oracle_base')
home = get_option('oracle_home')
base_path = base / home
oradb_bin = base_path / 'bin'
oci_path = base_path / 'oci'

if not fs.is_dir(oci_path)
    error('Oracle Client not found at `@0@`'.format(oci_path))
endif

oci_include = include_directories(oci_path / 'include')
oci_lib = cc.find_library('oci', dirs : [ oci_path / 'lib' / 'msvc' ])
oci_dll = files(oradb_bin / 'oci.dll')

oci = declare_dependency(
    include_directories : [ oci_include ],
    dependencies : [ oci_lib ]
)

meson.override_dependency('oci', oci)

install_data(oci_dll, install_dir : get_option('bindir'))
