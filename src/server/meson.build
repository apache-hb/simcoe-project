deps = [
    cthulhu.get_variable('format_core'),
    dependency('concurrentqueue'),
    core, system, threads, db,
    net, launch_gui, account,
]

src = [ 'src/common.cpp' ]

libserver_common = library('server-common', src,
    dependencies : deps
)

client_exe = executable('net-client', 'src/client.cpp',
    install : true,
    cpp_pch : 'src/stdafx.hpp',
    link_with : libserver_common,
    dependencies : deps
)

server_exe = executable('net-server', 'src/server.cpp',
    install : true,
    cpp_pch : 'src/stdafx.hpp',
    link_with : libserver_common,
    dependencies : [ deps ]
)

run_target('run-client',
    command : [ client_exe ]
)

run_target('run-server-gui',
    command : [ server_exe, '--mode=gui' ]
)

run_target('run-server-headless',
    command : [ server_exe, '--mode=headless' ]
)
