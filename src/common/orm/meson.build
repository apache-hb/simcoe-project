orm_include = include_directories('.', 'include')

# sqlite3 is always enabled
src = [
    'src/error.cpp',
    'src/connection.cpp',
    'src/transaction.cpp',

    'src/db/sqlite.cpp'
]

deps = [ core, dependency('sqlite3') ]
orm_args = [ '-DHAS_SQLITE3=1' ]

oradb_oci = subproject('oradb-oci', required : get_option('orcl_orm'))
if oradb_oci.found()
    src += 'src/db/oradb.cpp'
    deps += [ oradb_oci.get_variable('oci') ]
    orm_args += [ '-DHAS_ORACLEDB=1' ]
else
    orm_args += [ '-DHAS_ORACLEDB=0' ]
endif

db2cli = subproject('db2cli', required : get_option('db2_orm'))
if db2cli.found()
    src += 'src/db/db2.cpp'
    deps += [ db2cli.get_variable('db2cli') ]
    orm_args += [ '-DHAS_DB2=1' ]
else
    orm_args += [ '-DHAS_DB2=0' ]
endif

postgres = dependency('libpq', required : get_option('psql_orm'))
if postgres.found()
    src += 'src/db/postgres.cpp'
    deps += [ postgres ]
    orm_args += [ '-DHAS_POSTGRES=1' ]
else
    orm_args += [ '-DHAS_POSTGRES=0' ]
endif

if get_option('mysql_orm').allowed() and false
    src += 'src/db/mysql.cpp'
    orm_args += [ '-DHAS_MYSQL=1' ]
else
    orm_args += [ '-DHAS_MYSQL=0' ]
endif

if get_option('mssql_orm').allowed() and false
    src += 'src/db/mssql.cpp'
    orm_args += [ '-DHAS_MSSQL=1' ]
else
    orm_args += [ '-DHAS_MSSQL=0' ]
endif

liborm = library('orm', src,
    cpp_args : [ '-DSM_BUNDLE_BUILD=1', orm_args ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ orm_include, reflect_include, 'src' ],
    dependencies : deps
)

orm = declare_dependency(
    link_with : liborm,
    include_directories : [ orm_include ],
    dependencies : [ core ]
)


test('sqlite3 ORM',
    executable('orm-sqlite', 'test/sqlite.cpp', dependencies : [ orm, coretest ])
)

if oradb_oci.found()
    test('OracleDB ORM',
        executable('orm-orcl', 'test/orcl.cpp', dependencies : [ orm, coretest ])
    )
endif
