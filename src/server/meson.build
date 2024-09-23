src = [ files('src/main.cpp'), daocc.process('data/server.xml') ]
pch = meson.current_source_dir() / 'src/stdafx.hpp'

deps = [
    cthulhu.get_variable('format_core'),
    core, system, threads, db, structured_logs,
    packets, net
]
