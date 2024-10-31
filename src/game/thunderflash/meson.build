thunderflash_include = include_directories('.', 'include')

###
### reflection data
###

src = [
    daocc.process('data/thunderflash.xml'),
]

deps = [ core, db, dao ]

libthunderflash_meta = library('thunderflash-meta', src,
    include_directories : thunderflash_include,
    dependencies : deps,
)

thunderflash_meta = declare_dependency(
    link_with : libthunderflash_meta,
    include_directories : [ thunderflash_include, libthunderflash_meta.private_dir_include() ],
    dependencies : deps,
)

###
### implementation
###

src = [
    'src/thunderflash.cpp',
]

deps = [ logs, math, thunderflash_meta ]

libthunderflash = library('thunderflash', src,
    cpp_args : [ '-DSM_THUNDERFLASH_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : thunderflash_include,
    dependencies : deps,
)

thunderflash = declare_dependency(
    link_with : libthunderflash,
    include_directories : thunderflash_include,
    dependencies : deps,
)
