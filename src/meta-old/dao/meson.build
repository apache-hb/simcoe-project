xmlwrapp = dependency('libxml-2.0')

daoexe = executable('daocc', 'main.cpp',
    include_directories : 'include',
    cpp_pch : 'include/stdafx.hpp',
    dependencies : [ xmlwrapp, fmtlib ],
    override_options : [ 'cpp_rtti=true' ]
)

daocc = generator(daoexe,
    arguments : [ '@INPUT@', '--header', '@OUTPUT0@', '--source', '@OUTPUT1@', '-d', '@DEPFILE@' ],
    depfile : '@BASENAME@.d',
    output : [ '@BASENAME@.dao.h', '@BASENAME@.dao.cpp' ],
)
