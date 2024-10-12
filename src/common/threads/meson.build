threads_include = include_directories('.', 'include')

###
### reflection data
###

src = [
    daocc.process('data/topology.xml'),
    metacc.process('include/threads/threads.hpp'),
]

libthreads_meta = library('threads-meta', src,
    include_directories : threads_include,
    dependencies : [ core, meta, logs, db ],
)

threads_meta = declare_dependency(
    link_with : libthreads_meta,
    include_directories : [ threads_include, libthreads_meta.private_dir_include() ],
    dependencies : [ core, meta, logs, db ],
)

###
### thread api
###

src = [
    'src/threads.cpp',
    'src/geometry.cpp',
    'src/schedule.cpp',

    'src/cpuinfo.cpp',

    'src/backends/common.cpp',
    'src/backends/cpuset.cpp',
    'src/backends/logical.cpp',
    'src/backends/default.cpp',
]

deps = [ core, config, logs, logs, system, threads_meta ]

libthreads = library('threads', src,
    cpp_args : [ '-DSM_THREADS_BUILD=1' ],
    include_directories : [ threads_include, 'src' ],
    dependencies : deps
)

threads = declare_dependency(
    link_with : libthreads,
    include_directories : [ threads_include ],
    dependencies : deps
)

testcases = {
    'Processor geometry parsing': 'test/geometry.cpp',
    'Save processor topology data': 'test/topology.cpp',
}

foreach name, source : testcases
    exe = executable('test-threads-' + name, source,
        include_directories : 'src',
        dependencies : [ threads, coretest ]
    )

    test(name, exe,
        suite : 'threads',
        kwargs : testkwargs
    )
endforeach
