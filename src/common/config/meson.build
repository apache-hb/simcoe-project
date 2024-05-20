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

test('config cvar tests',
    executable('config-cvar-test', 'test/cvar.cpp',
        include_directories : inc,
        dependencies : testdeps
    )
)

# test('loading config from file',
#     executable('config-from-file-test', 'tests/config_from_file.cpp',
#         include_directories : inc,
#         dependencies : [ config, catch2 ]
#     )
# )
