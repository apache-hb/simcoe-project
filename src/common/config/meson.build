inc = include_directories('include')
tomlplusplus = dependency('tomlplusplus')

deps = [ core, logs ]

src = [
    'src/legacy_config.cpp',

    'src/options.cpp',
    'src/command_line.cpp',
    'src/config.cpp'
]

libconfig = library('config', src,
    include_directories : inc,
    cpp_args : [ '-DSM_CONFIG_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    dependencies : [ deps, tomlplusplus ]
)

config = declare_dependency(
    link_with : libconfig,
    include_directories : inc,
    dependencies : deps
)

catch2 = dependency('catch2-with-main')

testdeps = [
    config, catch2,
    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('format_core'),
    cthulhu.get_variable('io'),
]

test('options',
    executable('config-cvar-test', 'test/options.cpp',
        include_directories : inc,
        dependencies : testdeps
    ),
    suite : 'config'
)

test('command line parsing',
    executable('config-command-line-test', 'test/command_line.cpp',
        include_directories : inc,
        dependencies : testdeps
    ),
    suite : 'config'
)

# test('loading config from file',
#     executable('config-from-file-test', 'tests/config_from_file.cpp',
#         include_directories : inc,
#         dependencies : [ config, catch2 ]
#     )
# )
