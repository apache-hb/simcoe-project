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
    llvm,

    fmtlib,
]

src = [
    'src/main.cpp'
]

meta_tool = executable('meta-tool', src,
    include_directories : [ 'include' ],
    cpp_pch : 'src/stdafx.hpp',
    dependencies : deps,
    override_options : [ 'b_vscrt=mt' ],
    cpp_args : [ '-D_ITERATOR_DEBUG_LEVEL=0', '-D_SILENCE_CXX23_ALIGNED_UNION_DEPRECATION_WARNING=1' ]
)

meta_wrap = find_program('data/meta_wrap.py')

metacc = generator(meta_wrap,
    arguments : [
        '--builddir', meson.global_build_root(),
        '--sourcedir', '@CURRENT_SOURCE_DIR@',
        '--reflect', '@INPUT@',
        '--metatool', meta_tool.full_path(),
        '--header', '@OUTPUT0@',
        '--source', '@OUTPUT1@',
    ],
    depends : [ meta_tool ],
    depfile : '@BASENAME@.d',
    output : [ '@BASENAME@.meta.hpp', '@BASENAME@.meta.cpp' ],
)
