threads_include = include_directories('.', 'include')

src = [
    'src/threads.cpp',
    'src/geometry.cpp',
    'src/schedule.cpp',

    metacc.process('include/threads/threads.hpp')
]

deps = [ core, config, logs, structured_logs, system, meta ]

libthreads = library('threads', src,
    cpp_args : [ '-DSM_THREADS_BUILD=1' ],
    include_directories : [ threads_include ],
    dependencies : deps
)

threads = declare_dependency(
    link_with : libthreads,
    include_directories : [ threads_include, libthreads.private_dir_include() ],
    dependencies : deps
)

# TODO: re-enable tests
testcases = {
    # 'Processor geometry': 'test/geometry.cpp',
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
