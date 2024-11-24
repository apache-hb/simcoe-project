llvm = dependency('llvm', modules : [ 'core', 'support', 'option', 'frontendopenmp', 'windowsdriver' ], method : 'cmake', static : true)
llvm_libdir = llvm.get_variable(cmake : 'LLVM_LIBRARY_DIRS')

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
    cpp.find_library('version', required : false),
    llvm,

    fmtlib,

    dependency('zlib'),
    dependency('libzstd'),
    dependency('argo')
]

src = [
    'src/main.cpp'
]

meta_tool = executable('meta-tool', src,
    include_directories : [ 'include' ],
    cpp_pch : 'src/stdafx.hpp',
    dependencies : deps,
    override_options : [ 'b_vscrt=mt', 'b_sanitize=none' ],
    cpp_args : [
        '-D_ITERATOR_DEBUG_LEVEL=0',
        '-D_SILENCE_CXX23_ALIGNED_UNION_DEPRECATION_WARNING=1'
    ]
)

fs = import('fs')

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
    depends : [
        meta_tool,

        # TODO: this is a bit of a hack, depends cant accept files
        fs.copyfile('data/templates/header.meta.hpp.jinja2'),
        fs.copyfile('data/templates/source.meta.cpp.jinja2')
    ],
    depfile : '@BASENAME@.d',
    output : [ '@BASENAME@.meta.hpp', '@BASENAME@.meta.cpp' ],
)
