packets_include = include_directories('include')

src = [
    'src/packets.cpp',

    metacc.process('include/packets/packets.hpp'),
]

deps = [ core, meta ]

libpackets = library('packets', src,
    cpp_args : [ '-DSM_PACKETS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ packets_include, 'src' ],
    dependencies : deps
)

packets = declare_dependency(
    link_with : libpackets,
    include_directories : [ packets_include, libpackets.private_dir_include() ],
    dependencies : deps
)
