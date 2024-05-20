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
    'src/memory.c',
    'src/string.cpp',
    'src/error.cpp',
    'src/units.cpp',
    'src/utf8.cpp',
    'src/library.cpp',
    'src/timer.cpp',
    'src/typeindex.cpp',
    'src/core.cpp',

    'src/adt/small_string.cpp',

    'src/allocators/bitmap_allocator.cpp'
]

libcore = library('core', src,
    cpp_args : [ '-DSM_CORE_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ 'src', core_include, config_include, reflect_include ],
    dependencies : core_dependencies
)

core = declare_dependency(
    link_with : libcore,
    include_directories : [ core_include, config_include ],
    dependencies : core_dependencies
)
