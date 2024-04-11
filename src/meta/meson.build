llvm = dependency('llvm', components : ['core'], method : 'config-tool', static : true)
llvm_libdir = llvm.get_variable(configtool: 'libdir')

deps = [
    cpp.find_library('clangBasic', dirs : llvm_libdir),
    cpp.find_library('clangDriver', dirs : llvm_libdir),
    cpp.find_library('clangLex', dirs : llvm_libdir),
    cpp.find_library('clangParse', dirs : llvm_libdir),
    cpp.find_library('clangSema', dirs : llvm_libdir),
    cpp.find_library('clangAST', dirs : llvm_libdir),
    cpp.find_library('clangASTMatchers', dirs : llvm_libdir),
    cpp.find_library('clangAnalysis', dirs : llvm_libdir),
    cpp.find_library('clangAPINotes', dirs : llvm_libdir),
    cpp.find_library('clangFrontend', dirs : llvm_libdir),
    cpp.find_library('clangFrontendTool', dirs : llvm_libdir),
    cpp.find_library('clangSerialization', dirs : llvm_libdir),
    cpp.find_library('clangToolingCore', dirs : llvm_libdir),
    cpp.find_library('clangTooling', dirs : llvm_libdir),
    cpp.find_library('clangEdit', dirs : llvm_libdir),
    cpp.find_library('clangSupport', dirs : llvm_libdir),
    cpp.find_library('version'),
    llvm
]

src = [
    'src/main.cpp',
    'src/utils.cpp'
]

meta_tooling = executable('meta', src,
    include_directories : [ 'include', 'data/include' ],
    cpp_pch : 'src/stdafx.hpp',
    dependencies : deps,
    override_options : [ 'b_vscrt=mt' ],
    cpp_args : [ '-D_ITERATOR_DEBUG_LEVEL=0' ]
)

meta_include = include_directories('data/include')

cereal = dependency('cereal')

meta = declare_dependency(
    include_directories : meta_include,
    dependencies : cereal
)
