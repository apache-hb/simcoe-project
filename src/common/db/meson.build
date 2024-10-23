###
### configuration and driver detection
###

cfg = configuration_data()

tlsf = dependency('tlsf')
sqlite3 = dependency('sqlite3')
oci = dependency('instantclient', required : get_option('orcl'))
db2 = subproject('db2-client', required : get_option('db2'))
postgres = dependency('libpq', required : get_option('psql'))
devapi = dependency('devapi', required : get_option('mysql'))
has_mssql = get_option('mssql').allowed() and false
has_odbc = get_option('odbc').allowed() and false

cfg.set10('SMC_DB_HAS_SQLITE3', true)
cfg.set10('SMC_DB_HAS_ORCL', oci.found())
cfg.set10('SMC_DB_HAS_DB2', db2.found())
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
    metacc.process('include/db/db.hpp'),
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

    # helper classes
    'src/db/error.cpp',
    'src/db/bind.cpp',
    'src/db/results.cpp',
    'src/db/transaction.cpp',

    # the big three
    'src/db/statement.cpp',
    'src/db/connection.cpp',
    'src/db/environment.cpp',

    # shared driver utilities
    'src/db/drivers/common.cpp',
    'src/db/drivers/utils.cpp',
]

wil = dependency('wil')

deps = [ core, meta, logs, db_meta ]
db_private_deps = [ sqlite3, tlsf, wil ]

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
        'src/drivers/oracle/src/environment.cpp',
    ]
    db_private_include += [ include_directories('src/drivers/oracle/include') ]
    db_private_deps += [ oci ]
endif

if db2.found()
    src += [
        'src/drivers/db2/src/db2.cpp',
        'src/drivers/db2/src/statement.cpp',
        'src/drivers/db2/src/connection.cpp',
        'src/drivers/db2/src/environment.cpp',
    ]
    db_private_include += [ include_directories('src/drivers/db2/include') ]
    db_private_deps += [ db2.get_variable('db2cli') ]
endif

if postgres.found()
    src += 'src/db/drivers/postgres.cpp'
    db_private_deps += [ postgres ]
endif

if devapi.found()
    src += 'src/db/drivers/mysql.cpp'
    db_private_deps += [ devapi ]
endif

if has_mssql
    src += 'src/db/drivers/mssql.cpp'
endif

if has_odbc
    src += 'src/db/drivers/odbc.cpp'
endif

libdb = library('orm', [ src, db_config ],
    cpp_args : [ '-DSM_DB_BUILD=1', args, cpp.get_supported_arguments('-Wno-c99-designator', '-Wno-cast-function-type-mismatch') ],
    cpp_pch : 'src/db/stdafx.hpp',
    include_directories : [ db_include, db_private_include ],
    dependencies : [ deps, db_private_deps ]
)

db = declare_dependency(
    link_with : libdb,
    include_directories : [ '.', db_include ],
    dependencies : deps
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
### test util library
###

libdbtest = library('dbtest', 'test/db_test_common.cpp',
    include_directories : 'test',
    dependencies : [ coretest, db ]
)

dbtest = declare_dependency(
    link_with : libdbtest,
    include_directories : '.',
    dependencies : [ coretest, db ]
)

###
### tools
###

dbexec = executable('dbexec', 'src/tools/dbexec.cpp',
    include_directories : 'include',
    dependencies : [ core, dbtest ]
)

run_target('create-oracle-test-user',
    command : [
        dbexec, 'oracle', 'localhost', '1521',
        'sys', 'oracle', 'FREEPDB1', 'sysdba',
        '@CURRENT_SOURCE_DIR@/data/oracle/create-test-user.sql'
    ]
)

docker = find_program('docker', required : false)

if docker.found()
    compose_cfg = '@CURRENT_SOURCE_DIR@/data/docker/docker-compose.yml'

    run_target('start-oracledb',
        command : [ docker, 'compose', '-f', compose_cfg, 'up', '-d', 'oracle' ]
    )

    run_target('start-db2',
        command : [ docker, 'compose', '-f', compose_cfg, 'up', '-d', 'db2' ]
    )
endif

###
### test cases
###

tests = {
    'Sqlite3': 'test/db/sqlite.cpp',
    'Oracle DB': 'test/db/orcl.cpp',
    'PostgreSQL': 'test/db/psql.cpp',
    # TODO: disabled until i figure out a sane way to remap named parameters
    # onto positional parameters
    # 'IBM DB2': 'test/db/db2.cpp',
    'DAO': [ 'test/dao/dao.cpp', daocc.process('test/dao/tests.xml') ]
}

foreach name, source : tests
    exe = executable('test-db-' + name.to_lower().replace(' ', '-'), source,
        include_directories : 'test',
        dependencies : [ db, dbtest ]
    )

    test(name, exe,
        suite : 'db',
        kwargs : testkwargs
    )
endforeach
