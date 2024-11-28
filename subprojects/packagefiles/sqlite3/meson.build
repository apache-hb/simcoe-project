project('sqlite3', 'c',
    version : '3.4.6',
    meson_version: '>=1.1.0',
    license_files : 'LICENSE',
    license : 'sqlite'
)

src = [
    'alter.c',
    'analyze.c',
    'attach.c',
    'auth.c',
    'backup.c',
    'bitvec.c',
    'btmutex.c',
    'btree.c',
    'build.c',
    'callback.c',
    'complete.c',
    'ctime.c',
    'date.c',
    'dbpage.c',
    'dbstat.c',
    'delete.c',
    'expr.c',
    'fault.c',
    'fkey.c',
    'fts3.c',
    'fts3_aux.c',
    'fts3_expr.c',
    'fts3_hash.c',
    'fts3_icu.c',
    'fts3_porter.c',
    'fts3_snippet.c',
    'fts3_tokenizer.c',
    'fts3_tokenizer1.c',
    'fts3_tokenize_vtab.c',
    'fts3_unicode.c',
    'fts3_unicode2.c',
    'fts3_write.c',
    'fts5.c',
    'func.c',
    'global.c',
    'hash.c',
    'icu.c',
    'insert.c',
    'json.c',
    'legacy.c',
    'loadext.c',
    'main.c',
    'malloc.c',
    'mem0.c',
    'mem1.c',
    'mem2.c',
    'mem3.c',
    'mem5.c',
    'memdb.c',
    'memjournal.c',
    'mutex.c',
    'mutex_noop.c',
    'mutex_unix.c',
    'mutex_w32.c',
    'notify.c',
    'opcodes.c',
    'os.c',
    'os_kv.c',
    'os_unix.c',
    'os_win.c',
    'pager.c',
    'parse.c',
    'pcache.c',
    'pcache1.c',
    'pragma.c',
    'prepare.c',
    'printf.c',
    'random.c',
    'resolve.c',
    'rowset.c',
    'rtree.c',
    'select.c',
    'sqlite3rbu.c',
    'sqlite3session.c',
    'status.c',
    'stmt.c',
    'table.c',
    'threads.c',
    'tokenize.c',
    'treeview.c',
    'trigger.c',
    'update.c',
    'upsert.c',
    'userauth.c',
    'utf.c',
    'util.c',
    'vacuum.c',
    'vdbe.c',
    'vdbeapi.c',
    'vdbeaux.c',
    'vdbeblob.c',
    'vdbemem.c',
    'vdbesort.c',
    'vdbetrace.c',
    'vdbevtab.c',
    'vtab.c',
    'wal.c',
    'walker.c',
    'where.c',
    'wherecode.c',
    'whereexpr.c',
    'window.c'
]

features = {
    'memstatus': 'SQLITE_DEFAULT_MEMSTATUS',
    'synchronous_wal': 'SQLITE_DEFAULT_WAL_SYNCHRONOUS',
    'like_doesnt_match_blob': 'SQLITE_LIKE_DOESNT_MATCH_BLOBS',
    'omit_decltype': 'SQLITE_OMIT_DECLTYPE',
    'omit_deprecated': 'SQLITE_OMIT_DEPRECATED',
    'omit_progress_callback': 'SQLITE_OMIT_PROGRESS_CALLBACK',
    'omit_shared_cache': 'SQLITE_OMIT_SHARED_CACHE',
    'use_alloca': 'SQLITE_USE_ALLOCA',
    'omit_autoinit': 'SQLITE_OMIT_AUTOINIT',
    'strict_subtype': 'SQLITE_STRICT_SUBTYPE'
}

args = [
    '-DSQLITE_CORE=1',

    '-DSQLITE_OMIT_LOAD_EXTENSION=1',
    '-DSQLITE_OMIT_WAL=1',
    '-DSQLITE_OMIT_JSON=1',
    '-DSQLITE_OMIT_AUTOMATIC_INDEX=1',
    '-DSQLITE_OMIT_INCRBLOB=1',
    '-DSQLITE_OMIT_TCL_VARIABLE=1',
    '-DSQLITE_OMIT_UTF16=1',

    '-DSQLITE_DEFAULT_FOREIGN_KEYS=1',
    '-DSQLITE_ENABLE_MATH_FUNCTIONS=1',
    '-DSQLITE_HAVE_ISNAN=1',
    '-DSQLITE_UNTESTABLE=1',

    '-DSQLITE_MAX_EXPR_DEPTH=@0@'.format(get_option('max_expr_depth')),

    '-D_CRT_SECURE_NO_WARNINGS=1'
]

threadsafe = get_option('threadsafe')
if threadsafe == 'singlethreaded'
    args += '-DSQLITE_THREADSAFE=0'
elif threadsafe == 'multithreaded'
    args += '-DSQLITE_THREADSAFE=1'
elif threadsafe == 'serialized'
    args += '-DSQLITE_THREADSAFE=2'
endif

if get_option('disable_dqs')
    args += '-DSQLITE_DQS=0'
endif

foreach option, define : features
    if get_option(option)
        args += '-D@0@=1'.format(define)
    endif
endforeach

api_armor = get_option('api_armor').disable_auto_if(not get_option('buildtype').contains('debug'))

if api_armor.enabled()
    args += '-DSQLITE_ENABLE_API_ARMOR=1'
endif

libsqlite3 = library('sqlite3', src,
    c_args : args,
    include_directories : '.',
    override_options : [ 'unity=off' ]
)

sqlite3_dep = declare_dependency(
    link_with : libsqlite3,
    include_directories : '.',
)

meson.override_dependency('sqlite3', sqlite3_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
