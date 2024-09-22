project('db2-client', 'c',
    version : '19.0.0'
)

fs = import('fs')
cc = meson.get_compiler('c')

base = get_option('db2_home')

if base == '' or not fs.is_dir(base)
    error('DB2 installation not found at `@0@`'.format(base))
endif

db2_lib = cc.find_library('db2cli64', dirs : [ base / 'lib' ])
db2_include = include_directories(base / 'include')

db2cli = declare_dependency(
    include_directories : [ db2_include ],
    link_with : [ db2_lib ]
)

meson.override_dependency('db2cli', db2cli)

install_data(db2_lib, install_dir : get_option('bindir'))
