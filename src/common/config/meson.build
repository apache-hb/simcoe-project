inc = include_directories('include')
tomlplusplus = dependency('tomlplusplus')

deps = [ core ]

src = [
    'src/options.cpp',
    'src/command_line.cpp',
    'src/config.cpp',
]

libconfig = library('config', src,
    include_directories : inc,
    cpp_args : [ '-DSM_CONFIG_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    dependencies : [ deps, logs, tomlplusplus ],
    override_options : [ 'unity=off' ]
)

config = declare_dependency(
    link_with : libconfig,
    include_directories : inc,
    dependencies : deps
)

testdeps = [ config, coretest ]

test('options',
    executable('config-cvar', 'test/options.cpp',
        dependencies : testdeps
    ),
    suite : 'config',
    kwargs : testkwargs
)

test('command line parsing',
    executable('config-command-line', 'test/command_line.cpp',
        dependencies : testdeps
    ),
    suite : 'config',
    kwargs : testkwargs
)

test('loading config from file',
    executable('config-file', 'test/config.cpp',
        dependencies : testdeps
    ),
    suite : 'config',
    kwargs : testkwargs
)
