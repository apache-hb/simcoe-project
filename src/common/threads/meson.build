threads_include = include_directories('.', 'include')

src = [
    'src/threads.cpp',
    'src/geometry.cpp',
    'src/schedule.cpp',
]

deps = [ core, logs, system ]

libthreads = library('threads', src,
    cpp_args : [ '-DSM_THREADS_BUILD=1' ],
    include_directories : [ threads_include ],
    dependencies : deps
)

threads = declare_dependency(
    link_with : libthreads,
    include_directories : [ threads_include ],
    dependencies : deps
)
