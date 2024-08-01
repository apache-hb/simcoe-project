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

oradb_oci = subproject('oradb-oci', required : get_option('orcl_orm'))
if oradb_oci.found()
    src += 'src/orm/db/oradb.cpp'
    deps += [ oradb_oci.get_variable('oci') ]
    orm_args += [ '-DHAS_ORACLEDB=1' ]
else
    orm_args += [ '-DHAS_ORACLEDB=0' ]
endif

db2cli = subproject('db2cli', required : get_option('db2_orm'))
if db2cli.found()
    src += 'src/orm/db/db2.cpp'
    deps += [ db2cli.get_variable('db2cli') ]
    orm_args += [ '-DHAS_DB2=1' ]
else
    orm_args += [ '-DHAS_DB2=0' ]
endif

postgres = dependency('libpq', required : get_option('psql_orm'))
if postgres.found()
    src += 'src/orm/db/postgres.cpp'
    deps += [ postgres ]
    orm_args += [ '-DHAS_POSTGRES=1' ]
else
    orm_args += [ '-DHAS_POSTGRES=0' ]
endif

devapi = dependency('devapi', required : get_option('mysql_orm'))
if devapi.found()
    src += 'src/orm/db/mysql.cpp'
    deps += [ devapi ]
    orm_args += [ '-DHAS_MYSQL=1' ]
else
    orm_args += [ '-DHAS_MYSQL=0' ]
endif

if get_option('mssql_orm').allowed() and false
    src += 'src/orm/db/mssql.cpp'
    orm_args += [ '-DHAS_MSSQL=1' ]
else
    orm_args += [ '-DHAS_MSSQL=0' ]
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


test('sqlite3 ORM',
    executable('orm-sqlite', 'test/orm/sqlite.cpp', dependencies : [ orm, coretest ])
)

if oradb_oci.found()
    test('OracleDB ORM',
        executable('orm-orcl', 'test/orm/orcl.cpp', dependencies : [ orm, coretest ])
    )
endif

if postgres.found()
    test('PostgreSQL ORM',
        executable('orm-psql', 'test/orm/psql.cpp', dependencies : [ orm, coretest ])
    )
endif
