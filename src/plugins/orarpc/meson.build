src = [
    'src/orarpc.cpp',
    'src/ssl.cpp',
]

inc = include_directories('include')

deps = [
    core, db, net,
    dependency('concurrentqueue'),
    dependency('openssl'),
]

liborarpc = library('orarpc', src,
    include_directories : [ inc ],
    dependencies : deps
)

orarpc = declare_dependency(
    link_with : liborarpc,
    include_directories : [ inc ],
    dependencies : deps
)

if host_machine.system() == 'linux'
    exe = executable('orarpc-test', 'test/server.cpp',
        dependencies : [ orarpc, launch ],
        link_args : '-static'
    )

    run_target('run-orarpc-test', command : [ exe ])
endif
