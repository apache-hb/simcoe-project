threads_include = include_directories('.', 'include')

src = [
    'src/threads.cpp',
    reflect.process('data/threads.ct')
]

deps = [ core, logs, system ]

libthreads = library('threads', src,
    cpp_args : [ '-DSM_THREADS_BUILD=1' ],
    include_directories : [ threads_include, res_include ],
    dependencies : deps
)

threads = declare_dependency(
    link_with : libthreads,
    include_directories : [ threads_include, libthreads.private_dir_include() ],
    dependencies : deps
)
