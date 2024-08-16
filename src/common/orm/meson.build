orm_include = include_directories('.', 'include')

src = [
    'src/orm/error.cpp',
    'src/orm/connection.cpp',
    'src/orm/transaction.cpp',

    # sqlite3 is always enabled
    'src/orm/db/sqlite.cpp'
]

deps = [ core, dependency('sqlite3') ]
orm_args = [ '-DHAS_SQLITE3=1' ]

oradb_oci = subproject('orcl-oci', required : get_option('orcl'))
if oradb_oci.found()
    src += 'src/orm/db/orcl.cpp'
    deps += [ oradb_oci.get_variable('oci') ]
    orm_args += [ '-DHAS_ORCL=1' ]
else
    orm_args += [ '-DHAS_ORCL=0' ]
endif

db2cli = subproject('db2cli', required : get_option('db2'))
if db2cli.found()
    src += 'src/orm/db/db2.cpp'
    deps += [ db2cli.get_variable('db2cli') ]
    orm_args += [ '-DHAS_DB2=1' ]
else
    orm_args += [ '-DHAS_DB2=0' ]
endif

postgres = dependency('libpq', required : get_option('psql'))
if postgres.found()
    src += 'src/orm/db/postgres.cpp'
    deps += [ postgres ]
    orm_args += [ '-DHAS_POSTGRES=1' ]
else
    orm_args += [ '-DHAS_POSTGRES=0' ]
endif

devapi = dependency('devapi', required : get_option('mysql'))
if devapi.found()
    src += 'src/orm/db/mysql.cpp'
    deps += [ devapi ]
    orm_args += [ '-DHAS_MYSQL=1' ]
else
    orm_args += [ '-DHAS_MYSQL=0' ]
endif

if get_option('mssql').allowed() and false
    src += 'src/orm/db/mssql.cpp'
    orm_args += [ '-DHAS_MSSQL=1' ]
else
    orm_args += [ '-DHAS_MSSQL=0' ]
endif

if get_option('odbc').allowed() and false
    src += 'src/orm/db/odbc.cpp'
    orm_args += [ '-DHAS_ODBC=1' ]
else
    orm_args += [ '-DHAS_ODBC=0' ]
endif

liborm = library('orm', src,
    cpp_args : [ '-DSM_BUNDLE_BUILD=1', orm_args ],
    cpp_pch : 'src/orm/stdafx.hpp',
    include_directories : [ orm_include, reflect_include, 'src/orm' ],
    dependencies : deps
)

orm = declare_dependency(
    link_with : liborm,
    include_directories : [ orm_include ],
    dependencies : [ core ]
)

tests = {
    'sqlite3': 'test/orm/sqlite.cpp',
    'Oracle DB': 'test/orm/orcl.cpp',
    'PostgreSQL': 'test/orm/psql.cpp',
}

foreach name, source : tests
    exe = executable('test-orm-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ orm, coretest ]
    )

    test(name + ' ORM', exe,
        suite : 'orm',
        kwargs : testkwargs
    )
endforeach

llvm = dependency('llvm', components : ['core'], method : 'config-tool', static : true)
libxml = dependency('libxml-2.0')
llvm_libdir = llvm.get_variable(configtool: 'libdir')

deps = [
    fmtlib,
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

    libxml,
    fmtlib
]

src = [ 'src/daocc/main.cpp' ]

daoexe = executable('daocc', src,
    include_directories : 'include',
    cpp_pch : 'src/daocc/stdafx.hpp',
    cpp_args : [ '-D_SILENCE_CXX23_ALIGNED_UNION_DEPRECATION_WARNING=1', '-D_ITERATOR_DEBUG_LEVEL=0' ],
    dependencies : deps,
    override_options : [ 'b_vscrt=mt' ]
)

daocc = generator(daoexe,
    arguments : [ '@INPUT@', '--header', '@OUTPUT0@', '--source', '@OUTPUT1@', '-d', '@DEPFILE@' ],
    depfile : '@BASENAME@.d',
    output : [ '@BASENAME@.dao.h', '@BASENAME@.dao.cpp' ],
)
