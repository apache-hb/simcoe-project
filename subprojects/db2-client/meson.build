project('db2-client', 'c',
    version : '11.5.0'
)

fs = import('fs')
cc = meson.get_compiler('c')

base = get_option('home')

if base == '' or not fs.is_dir(base)
    error('DB2 installation not found at `@0@`'.format(base))
endif

db2_lib = cc.find_library('db2cli', dirs : [ base / 'lib' ])
db2_include = include_directories(base / 'include')

db2cli = declare_dependency(
    include_directories : [ db2_include ],
    dependencies : [ db2_lib ]
)
