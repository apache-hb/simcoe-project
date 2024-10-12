###
### configuration and driver detection
###

cfg = configuration_data()

tlsf = dependency('tlsf')
sqlite3 = dependency('sqlite3')
oci = dependency('instantclient', required : get_option('orcl'))
db2cli = subproject('db2-client', required : get_option('db2'))
postgres = dependency('libpq', required : get_option('psql'))
devapi = dependency('devapi', required : get_option('mysql'))
has_mssql = get_option('mssql').allowed() and false
has_odbc = get_option('odbc').allowed() and false

cfg.set10('SMC_DB_HAS_SQLITE3', true)
cfg.set10('SMC_DB_HAS_ORCL', oci.found())
cfg.set10('SMC_DB_HAS_DB2', db2cli.found())
cfg.set10('SMC_DB_HAS_POSTGRES', postgres.found())
cfg.set10('SMC_DB_HAS_MYSQL', devapi.found())
cfg.set10('SMC_DB_HAS_MSSQL', has_mssql)
cfg.set10('SMC_DB_HAS_ODBC', has_odbc)

db_config = configure_file(
    output : 'simcoe_db_config.h',
    configuration : cfg
)

###
### reflection data
###

db_include = include_directories('include')

src = [
    metacc.process('include/db/core.hpp'),
]

libdb_meta = library('db-meta', [ src, db_config ],
    include_directories : db_include,
    dependencies : [ core, meta, logs ]
)

db_meta = declare_dependency(
    link_with : libdb_meta,
    include_directories : [ db_include, libdb_meta.private_dir_include() ],
    dependencies : [ core, meta ]
)

###
### private impl and public api
###

db_private_include = [ include_directories('src/db', 'src/db/drivers') ]

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
]

deps = [ core, sqlite3, tlsf, meta, logs, db_meta ]

# sqlite3 is always enabled
src += [
    'src/drivers/sqlite/src/sqlite.cpp',
    'src/drivers/sqlite/src/statement.cpp',
    'src/drivers/sqlite/src/connection.cpp',
]

db_private_include += [ include_directories('src/drivers/sqlite/include') ]

if oci.found()
    src += [
        'src/drivers/oracle/src/oracle.cpp',
        'src/drivers/oracle/src/statement.cpp',
        'src/drivers/oracle/src/connection.cpp',
        'src/drivers/oracle/src/environment.cpp'
    ]
    db_private_include += [ include_directories('src/drivers/oracle/include') ]
    deps += [ oci ]
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

libdb = library('orm', [ src, db_config ],
    cpp_args : [ '-DSM_DB_BUILD=1', args, cpp.get_supported_arguments('-Wno-c99-designator') ],
    cpp_pch : 'src/db/stdafx.hpp',
    include_directories : [ db_include, db_private_include ],
    dependencies : deps
)

db = declare_dependency(
    link_with : libdb,
    include_directories : [ '.', db_include ],
    dependencies : [ core, meta, logs, db_meta ]
)

###
### dao code generation
###

src = [ 'src/daocc/main.cpp' ]

libxml = dependency('libxml-2.0')

dao = declare_dependency(
    include_directories : 'include',
)

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

###
### tests
###

tests = {
    'sqlite3': 'test/db/sqlite.cpp',
    'Oracle DB': 'test/db/orcl.cpp',
    'PostgreSQL': 'test/db/psql.cpp',
    'DAO': [ 'test/dao/dao.cpp', daocc.process('test/dao/tests.xml') ]
}

foreach name, source : tests
    exe = executable('test-db-' + name.to_lower().replace(' ', '-'), source,
        include_directories : 'test',
        dependencies : [ db, coretest ]
    )

    test(name + ' DB', exe,
        suite : 'db',
        kwargs : testkwargs
    )
endforeach
