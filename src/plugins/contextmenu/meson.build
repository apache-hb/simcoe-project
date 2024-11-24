src = [
    daocc.process('data/contextmenu.xml')
]

libcontextmenu_meta = library('contextmenu-meta', src,
    dependencies : [ db, dao ]
)

contextmenu_meta = declare_dependency(
    link_with : libcontextmenu_meta,
    include_directories : libcontextmenu_meta.private_dir_include(),
    dependencies : [ db, dao ]
)

src = [
    'src/contextmenu.cpp'
]

inc = include_directories('include')

deps = [ system, contextmenu_meta ]

libcontextmenu = library('contextmenu', src,
    include_directories : inc,
    dependencies : deps,
)

contextmenu = declare_dependency(
    link_with : libcontextmenu,
    include_directories : inc,
    dependencies : deps,
)

exe = executable('contextmenu-test', 'test/tool.cpp',
    dependencies : [ contextmenu, launch_gui ],
)

run_target('run-contextmenu-test', command : [ exe ])
