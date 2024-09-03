orm_include = include_directories('include')

src = [
    'src/orm/error.cpp',
    'src/orm/connection.cpp',
    'src/orm/transaction.cpp',

    'src/orm/db/common.cpp',

    # sqlite3 is always enabled
    'src/orm/db/sqlite.cpp',

    metacc.process('include/orm/core.hpp')
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

deps = [ core, sqlite3, meta ]

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
    include_directories : [ '.', orm_include, liborm.private_dir_include() ],
    dependencies : [ core, meta ]
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

src = [ 'src/dao/utils.cpp' ]

libdao = library('dao', src,
    include_directories : 'include',
    dependencies : orm
)

dao = declare_dependency(
    include_directories : 'include',
    dependencies : orm,
    link_with : libdao
)

src = [ 'src/daocc/main.cpp' ]

libxml = dependency('libxml-2.0')

daocc_exe = executable('daocc', src,
    cpp_pch : 'src/daocc/stdafx.hpp',
    include_directories : 'include',
    dependencies : [ core, libxml ]
)

daocc = generator(daocc_exe,
    arguments : [ '@INPUT@', '--header', '@OUTPUT0@', '--source', '@OUTPUT1@', '-d', '@DEPFILE@', '@EXTRA_ARGS@' ],
    output : [ '@BASENAME@.dao.hpp', '@BASENAME@.dao.cpp' ],
    depfile : '@BASENAME@.d'
)
