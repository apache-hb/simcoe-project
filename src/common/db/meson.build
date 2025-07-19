###
### configuration and driver detection
###

cfg = configuration_data()

tlsf = dependency('tlsf')
sqlite3 = dependency('sqlite3')
oci = dependency('instantclient-' + host_machine.system(), required : get_option('orcl'))
oci_headers = dependency('instantclient-' + host_machine.system() + '-headers', required : get_option('orcl'))
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
### private impl and public api
###

db_include = include_directories('include')

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

deps = [ core, logs ]
db_private_deps = [ sqlite3, tlsf ]

# sqlite3 is always enabled
src += [
    'src/drivers/sqlite/src/sqlite.cpp',
    'src/drivers/sqlite/src/statement.cpp',
    'src/drivers/sqlite/src/connection.cpp',
    'src/drivers/sqlite/src/environment.cpp',
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
    db_private_deps += [ oci, oci_headers ]
endif

if db2.found()
    wil = dependency('wil')

    src += [
        'src/drivers/db2/src/db2.cpp',
        'src/drivers/db2/src/statement.cpp',
        'src/drivers/db2/src/connection.cpp',
        'src/drivers/db2/src/environment.cpp',
    ]
    db_private_include += [ include_directories('src/drivers/db2/include') ]
    db_private_deps += [ db2.get_variable('db2cli'), wil ]
endif

if postgres.found()
    src += [
        'src/drivers/postgres/src/postgres.cpp'
    ]
    db_private_include += [ include_directories('src/drivers/postgres/include') ]
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

args = cpp.get_supported_arguments(
    '-Wno-c99-designator',
    '-Wno-cast-function-type-mismatch',

    # bogus gcc warning
    '-Wno-subobject-linkage'
)

libdb = library('db', [ src, db_config ],
    cpp_args : [ '-DSM_DB_BUILD=1', args ],
    cpp_pch : 'src/db/stdafx.hpp',
    include_directories : [ db_include, db_private_include ],
    dependencies : [ deps, db_private_deps ],
    override_options : [ 'unity=off' ]
)

db = declare_dependency(
    link_with : libdb,
    include_directories : [ '.', db_include ],
    compile_args : args,
    dependencies : deps
)

###
### dao code generation
###

src = [ 'src/daocc/main.cpp' ]

argparse = dependency('argparse')
libxml = dependency('libxml-2.0')

dao = declare_dependency(
    include_directories : 'include',
)

daocc_exe = executable('daocc', src,
    cpp_pch : 'src/daocc/stdafx.hpp',
    cpp_args : cpp.get_supported_arguments('-Wno-unused-variable'),
    include_directories : 'include',
    dependencies : [ core, libxml, argparse ]
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

docker_exe = find_program('docker', required : false)

if docker_exe.found()
    compose_cfg = '@CURRENT_SOURCE_DIR@/data/docker/docker-compose.yml'

    run_target('start-oracledb',
        command : [ docker_exe, 'compose', '-f', compose_cfg, 'up', '-d', 'oracle' ]
    )

    run_target('start-db2',
        command : [ docker_exe, 'compose', '-f', compose_cfg, 'up', '-d', 'db2' ]
    )
endif

###
### test cases
###

tests = {
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
        kwargs : testkwargs + { 'timeout': 10 }
    )
endforeach

tests = {
    'Sqlite3': 'test/db/sqlite.cpp',
}

foreach name, source : tests
    exe = executable('test-db-' + name.to_lower().replace(' ', '-'), source,
        include_directories : 'test',
        cpp_args : cpp.get_supported_arguments('-Wno-sign-compare'),
        dependencies : [ db, coregtest ]
    )

    test(name, exe,
        suite : 'db',
        kwargs : gtestkwargs
    )
endforeach
