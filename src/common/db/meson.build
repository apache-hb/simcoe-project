db_include = include_directories('include')

src = [
    # dao support structures
    'src/dao/dao.cpp',

    # public api
    'src/db/error.cpp',
    'src/db/bind.cpp',
    'src/db/results.cpp',
    'src/db/statement.cpp',
    'src/db/connection.cpp',
    'src/db/transaction.cpp',

    # shared driver utilities
    'src/db/drivers/common.cpp',
    'src/db/drivers/utils.cpp',

    # sqlite3 is always enabled
    'src/db/drivers/sqlite/sqlite.cpp',
    'src/db/drivers/sqlite/statement.cpp',
    'src/db/drivers/sqlite/connection.cpp',

    metacc.process('include/db/core.hpp')
]

cfg = configuration_data()

tlsf = dependency('tlsf')
sqlite3 = dependency('sqlite3')
oracledb = subproject('oracledb-client', required : get_option('orcl'))
db2cli = subproject('db2cli', required : get_option('db2'))
postgres = dependency('libpq', required : get_option('psql'))
devapi = dependency('devapi', required : get_option('mysql'))
has_mssql = get_option('mssql').allowed() and false
has_odbc = get_option('odbc').allowed() and false

cfg.set10('SMC_DB_HAS_SQLITE3', true)
cfg.set10('SMC_DB_HAS_ORCL', oracledb.found())
cfg.set10('SMC_DB_HAS_DB2', db2cli.found())
cfg.set10('SMC_DB_HAS_POSTGRES', postgres.found())
cfg.set10('SMC_DB_HAS_MYSQL', devapi.found())
cfg.set10('SMC_DB_HAS_MSSQL', has_mssql)
cfg.set10('SMC_DB_HAS_ODBC', has_odbc)

deps = [ core, sqlite3, tlsf, meta, logs ]

if oracledb.found()
    src += [
        'src/db/drivers/orcl/orcl.cpp',
        'src/db/drivers/orcl/statement.cpp',
        'src/db/drivers/orcl/connection.cpp'
    ]
    deps += [ oracledb.get_variable('oci') ]
endif

if db2cli.found()
    src += 'src/db/drivers/db2.cpp'
    deps += [ db2cli.get_variable('db2cli') ]
endif

if postgres.found()
    src += 'src/db/drivers/postgres.cpp'
    deps += [ postgres ]
endif

if devapi.found()
    src += 'src/db/drivers/mysql.cpp'
    deps += [ devapi ]
endif

if has_mssql
    src += 'src/db/drivers/mssql.cpp'
endif

if has_odbc
    src += 'src/db/drivers/odbc.cpp'
endif

db_config = configure_file(
    output : 'simcoe_db_config.h',
    configuration : cfg
)

libdb = library('orm', [ src, db_config ],
    cpp_args : [ '-DSM_DB_BUILD=1', args, cpp.get_supported_arguments('-Wno-c99-designator') ],
    cpp_pch : 'src/db/stdafx.hpp',
    include_directories : [ db_include, reflect_include, 'src/db' ],
    dependencies : deps
)

db = declare_dependency(
    link_with : libdb,
    include_directories : [ '.', db_include, libdb.private_dir_include() ],
    dependencies : [ core, meta ]
)

tests = {
    'sqlite3': 'test/db/sqlite.cpp',
    'Oracle DB': 'test/db/orcl.cpp',
    'PostgreSQL': 'test/db/psql.cpp',
}

foreach name, source : tests
    exe = executable('test-db-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ db, coretest ]
    )

    test(name + ' DB', exe,
        suite : 'db',
        kwargs : testkwargs
    )
endforeach

src = [ 'src/daocc/main.cpp' ]

libxml = dependency('libxml-2.0')

daocc_exe = executable('daocc', src,
    cpp_pch : 'src/daocc/stdafx.hpp',
    cpp_args : cpp.get_supported_arguments('-Wno-unused-variable'),
    include_directories : 'include',
    dependencies : [ core, libxml ]
)

daocc = generator(daocc_exe,
    arguments : [ '@INPUT@', '--header', '@OUTPUT0@', '--source', '@OUTPUT1@', '-d', '@DEPFILE@', '@EXTRA_ARGS@' ],
    output : [ '@BASENAME@.dao.hpp', '@BASENAME@.dao.cpp' ],
    depfile : '@BASENAME@.d'
)
