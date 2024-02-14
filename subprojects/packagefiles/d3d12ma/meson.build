project('d3d12ma', 'cpp',
    license : 'MIT'
)

src = [
    'src/D3D12MemAlloc.cpp'
]

cpp = meson.get_compiler('cpp')

args = cpp.get_supported_arguments('-Wno-switch', '-Wno-tautological-undefined-compare')

add_project_arguments(args, language : 'cpp')

include = include_directories('include')

d3d12 = dependency('d3d12')

libmemalloc = library('d3d12ma', src,
    include_directories : include,
    dependencies : d3d12
)

d3d12ma_dep = declare_dependency(
    include_directories : include,
    link_with : libmemalloc,
    dependencies : d3d12
)
