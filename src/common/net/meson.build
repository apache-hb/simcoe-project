net_include = include_directories('.', 'include')

src = [
    'src/net.cpp'
]

ws2 = cc.find_library('ws2_32', required : is_windows)
deps = [ core, structured_logs, ws2, cthulhu.get_variable('os') ]

libnet = library('net', src,
    include_directories : [ net_include ],
    dependencies : deps
)

net = declare_dependency(
    link_with : libnet,
    include_directories : [ net_include ],
    dependencies : [ core ]
)

exe = executable('test-net', 'test/net.cpp',
    dependencies : [ net, coretest ]
)

test('Net', exe,
    suite : 'net',
    kwargs : testkwargs
)
