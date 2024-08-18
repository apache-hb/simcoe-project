orm_include = include_directories('.', 'include')

src = [
    'src/orm/error.cpp',
    'src/orm/connection.cpp',
    'src/orm/transaction.cpp',

    'src/orm/db/common.cpp',

    # sqlite3 is always enabled
    'src/orm/db/sqlite.cpp'
]

cfg = configuration_data()

sqlite3 = dependency('sqlite3')
oradb_oci = subproject('orcl-oci', required : get_option('orcl'))
db2cli = subproject('db2cli', required : get_option('db2'))
postgres = dependency('libpq', required : get_option('psql'))
devapi = dependency('devapi', required : get_option('mysql'))
has_mssql = get_option('mssql').allowed() and false
has_odbc = get_option('odbc').allowed() and false

cfg.set10('ORM_HAS_SQLITE3', true)
cfg.set10('ORM_HAS_ORCL', oradb_oci.found())
cfg.set10('ORM_HAS_DB2', db2cli.found())
cfg.set10('ORM_HAS_POSTGRES', postgres.found())
cfg.set10('ORM_HAS_MYSQL', devapi.found())
cfg.set10('ORM_HAS_MSSQL', has_mssql)
cfg.set10('ORM_HAS_ODBC', has_odbc)

deps = [ core, sqlite3 ]

if oradb_oci.found()
    src += [
        'src/orm/db/orcl/orcl.cpp',
        'src/orm/db/orcl/statement.cpp',
        'src/orm/db/orcl/connection.cpp'
    ]
    deps += [ oradb_oci.get_variable('oci') ]
endif

if db2cli.found()
    src += 'src/orm/db/db2.cpp'
    deps += [ db2cli.get_variable('db2cli') ]
endif

if postgres.found()
    src += 'src/orm/db/postgres.cpp'
    deps += [ postgres ]
endif

if devapi.found()
    src += 'src/orm/db/mysql.cpp'
    deps += [ devapi ]
endif

if has_mssql
    src += 'src/orm/db/mssql.cpp'
endif

if has_odbc
    src += 'src/orm/db/odbc.cpp'
endif

orm_config = configure_file(
    output : 'simcoe_orm_config.h',
    configuration : cfg
)

liborm = library('orm', [ src, orm_config ],
    cpp_args : [ '-DSM_BUNDLE_BUILD=1', args ],
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

libxml = dependency('libxml-2.0')
# llvm = dependency('llvm', components : ['core'], method : 'config-tool', static : true)
# llvm_libdir = llvm.get_variable(configtool: 'libdir')

deps = [
    # cpp.find_library('clangBasic', dirs : llvm_libdir),
    # cpp.find_library('clangDriver', dirs : llvm_libdir),
    # cpp.find_library('clangLex', dirs : llvm_libdir),
    # cpp.find_library('clangParse', dirs : llvm_libdir),
    # cpp.find_library('clangSema', dirs : llvm_libdir),
    # cpp.find_library('clangAST', dirs : llvm_libdir),
    # cpp.find_library('clangASTMatchers', dirs : llvm_libdir),
    # cpp.find_library('clangAnalysis', dirs : llvm_libdir),
    # cpp.find_library('clangAPINotes', dirs : llvm_libdir),
    # cpp.find_library('clangFrontend', dirs : llvm_libdir),
    # cpp.find_library('clangFrontendTool', dirs : llvm_libdir),
    # cpp.find_library('clangSerialization', dirs : llvm_libdir),
    # cpp.find_library('clangToolingCore', dirs : llvm_libdir),
    # cpp.find_library('clangTooling', dirs : llvm_libdir),
    # cpp.find_library('clangEdit', dirs : llvm_libdir),
    # cpp.find_library('clangSupport', dirs : llvm_libdir),
    # cpp.find_library('version'),
    # llvm,

    libxml,
    fmtlib,
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
    output : [ '@BASENAME@.dao.hpp', '@BASENAME@.dao.cpp' ],
)
