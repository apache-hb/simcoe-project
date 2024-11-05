###
### implementation
###

net_include = include_directories('.', 'include')

src = [
    'src/error.cpp',
    'src/address.cpp',

    'src/net.cpp',
    'src/socket.cpp',
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

###
### test utils
###

nettest_lib = static_library('nettest', 'test/net_test_common.cpp',
    include_directories : '.',
    dependencies : [ net, coretest ]
)

nettest = declare_dependency(
    link_with : nettest_lib,
    include_directories : '.',
    dependencies : [ coretest, net ]
)

###
### tests
###

testcases = {
    'Client Server communication': 'test/client_server.cpp',
    'Timeout on client recv': 'test/timeout_recv.cpp',
    'Timeout on connect to oversubcribed server': 'test/timeout_connect.cpp',
}

foreach name, source : testcases
    exe = executable('test-net-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ nettest ]
    )

    test(name, exe,
        suite : 'net',
        kwargs : testkwargs
    )
endforeach
