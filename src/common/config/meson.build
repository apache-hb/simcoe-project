inc = include_directories('include')

deps = [ core, logs ]

src = [
    'src/config.cpp',
    'src/cvar.cpp'
]

libconfig = library('config', src,
    cpp_args : [ '-DSM_CONFIG_BUILD=1' ],
    include_directories : inc,
    dependencies : deps
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
