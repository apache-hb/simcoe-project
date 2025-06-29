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
    'src/status.cpp',
    'src/threads.cpp',
    'src/topology.cpp',
]

if host_machine.system() == 'windows'
    src += [
        'src/save.cpp',
        'src/mutex.cpp',
        'src/cpuinfo.cpp',
    ]
else
    src += [
        'src/linux_save.cpp'
    ]
endif

deps = [ core, config, logs, logs, system, db, threads_meta ]

libthreads = library('threads', src,
    cpp_args : [ '-DSM_THREADS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ threads_include, 'src' ],
    dependencies : [ hwloc, deps ],
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
    'Save processor topology data': 'test/topology.cpp',
    # 'Triplebuffer': 'test/triplebuffer.cpp',
}

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

if host_machine.system() == 'windows'
    testcases = {
        'Processor geometry saving': 'test/cpuinfo.cpp',
    }

    foreach name, source : testcases
        exe = executable('test-threads-' + name.to_lower().replace(' ', '-'), source,
            include_directories : 'src',
            dependencies : [ threads, gtest ]
        )

        test(name, exe,
            suite : 'threads',
            kwargs : gtestkwargs
        )
    endforeach
endif

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
