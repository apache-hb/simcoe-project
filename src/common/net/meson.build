net_include = include_directories('.', 'include')

src = [
    'src/net.cpp'
]

ws2 = cc.find_library('ws2_32', required : is_windows)
deps = [ core, logs, ws2, cthulhu.get_variable('os') ]

libnet = library('net', src,
    include_directories : [ net_include ],
    dependencies : deps
)

net = declare_dependency(
    link_with : libnet,
    include_directories : [ net_include ],
    dependencies : [ core ]
)

testcases = {
    'Client Server communication': 'test/client_server.cpp',
    'Timeout on client recv': 'test/timeout_recv.cpp',
}

foreach name, source : testcases
    exe = executable('test-net-' + name, source,
        dependencies : [ net, coretest ]
    )

    test(name, exe,
        suite : 'net',
        kwargs : testkwargs
    )
endforeach
