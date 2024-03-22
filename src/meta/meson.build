llvm_dir = get_option('llvm_dir')
llvm_libdir = llvm_dir / 'lib'
llvm_incdir = llvm_dir / 'include'

deps = [
    cpp.find_library('clang', dirs : llvm_libdir)
]

meta_tooling = executable('meta', 'src/main.cpp',
    cpp_pch : 'src/stdafx.hpp',
    dependencies : deps,
    include_directories : [llvm_incdir],
    override_options : [ 'b_vscrt=md' ],
    cpp_args : [ '-D_ITERATOR_DEBUG_LEVEL=0' ]
)
