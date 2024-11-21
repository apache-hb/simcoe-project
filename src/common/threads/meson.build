threads_include = include_directories('.', 'include')

###
### reflection data
###

src = [
    daocc.process('data/topology.xml'),
]

libthreads_meta = library('threads-meta', src,
    include_directories : threads_include,
    dependencies : [ core, dao ],
)

threads_meta = declare_dependency(
    link_with : libthreads_meta,
    include_directories : [ threads_include, libthreads_meta.private_dir_include() ],
    dependencies : [ core, dao ],
)

###
### thread api
###

hwloc = dependency('hwloc')

src = [
    'src/scheduler.cpp',
]

if host_machine.system() == 'windows'
    src += [
        'src/topology.cpp',
        'src/threads.cpp',
        'src/cpuinfo.cpp',

        'src/backends/common.cpp',
        'src/backends/cpuset.cpp',
        'src/backends/logical.cpp',
        'src/backends/default.cpp',
    ]
endif

deps = [ core, config, logs, logs, system, db, hwloc, threads_meta ]

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

###
### tests
###

testcases = {
    'Nonblocking mailbox': 'test/mailbox.cpp',
}

if host_machine.system() == 'windows'
    testcases += {
        'Processor geometry parsing': 'test/geometry.cpp',
        'Save processor topology data': 'test/topology.cpp',
    }
endif

foreach name, source : testcases
    exe = executable('test-threads-' + name.to_lower().replace(' ', '-'), source,
        include_directories : 'src',
        dependencies : [ threads, dbtest ]
    )

    test(name, exe,
        suite : 'threads',
        kwargs : testkwargs + { 'timeout': 10 }
    )
endforeach

###
### benchmarks
###

googlebench = dependency('benchmark')

benchcases = {
    'Mailbox': 'benchmark/mailbox.cpp',
}

foreach name, source : benchcases
    exe = executable('bench-threads-' + name.to_lower().replace(' ', '-'), source,
        include_directories : 'src',
        dependencies : [ threads, coretest ]
    )

    benchmark(name, exe,
        suite : 'threads',
        kwargs : benchkwargs
    )
endforeach
