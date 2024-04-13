src = [
    'src/runtime.cpp'
]

inc = include_directories('include')

deps = [ meta_shared ]

libmeta = library('meta', src,
    include_directories : inc,
    cpp_pch : 'src/stdafx.hpp',
    dependencies : deps
)

meta = declare_dependency(
    include_directories : inc,
    link_with : libmeta,
    dependencies : deps
)
