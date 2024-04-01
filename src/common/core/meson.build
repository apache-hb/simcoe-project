core_include = include_directories('.', 'include')

core_dependencies = [
    fmtlib,
    cthulhu.get_variable('core'),
    cthulhu.get_variable('base'),
    cthulhu.get_variable('arena'),
    cthulhu.get_variable('std'),
    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('io'),
    cthulhu.get_variable('os'),
]

src = [
    'src/arena.cpp',
    'src/string.cpp',
    'src/error.cpp',
    'src/bitmap.cpp',
    'src/units.cpp',
    'src/utf8.cpp',
    'src/library.cpp',
    'src/timer.cpp',
    reflect.process('data/core.ct')
]

libcore = library('core', src,
    cpp_args : [ '-DSM_CORE_BUILD=1' ],
    include_directories : [ core_include, config_include, reflect_include ],
    dependencies : core_dependencies
)

core = declare_dependency(
    link_with : libcore,
    include_directories : [
        core_include, config_include,
        reflect_include, libcore.private_dir_include()
    ],
    dependencies : core_dependencies
)
