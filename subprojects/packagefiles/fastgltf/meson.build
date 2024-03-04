project('fastgltf', 'cpp')

simdjson = dependency('simdjson')

src = [ 'src/fastgltf.cpp', 'src/base64.cpp' ]
inc = include_directories('include')

libgltf = library('fastgltf', src,
    dependencies : simdjson,
    cpp_args : [ '-D_SILENCE_CXX20_U8PATH_DEPRECATION_WARNING' ],
    include_directories : inc
)

fastgltf_dep = declare_dependency(
    include_directories : inc,
    link_with : libgltf
)
