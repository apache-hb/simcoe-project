core_include = include_directories('.', 'include')

core_dependencies = [
    cthulhu.get_variable('core'),
    cthulhu.get_variable('base'),
    cthulhu.get_variable('arena'),
    cthulhu.get_variable('std'),
    cthulhu.get_variable('backtrace'),
    cthulhu.get_variable('io'),
]

src = [
    'src/arena.cpp',
    'src/text.cpp',
    'src/backtrace.cpp',
    'src/bitmap.cpp',
    'src/units.cpp',
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
