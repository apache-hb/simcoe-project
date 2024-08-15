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

test('sqlite3 ORM',
    executable('orm-sqlite', 'test/orm/sqlite.cpp', dependencies : [ orm, coretest ]),
    suite : 'orm',
    kwargs : testkwargs
)

if oradb_oci.found()
    test('OracleDB ORM',
        executable('orm-orcl', 'test/orm/orcl.cpp', dependencies : [ orm, coretest ]),
        suite : 'orm',
        kwargs : testkwargs
    )
endif

if postgres.found()
    test('PostgreSQL ORM',
        executable('orm-psql', 'test/orm/psql.cpp', dependencies : [ orm, coretest ]),
        suite : 'orm',
        kwargs : testkwargs
    )
endif
