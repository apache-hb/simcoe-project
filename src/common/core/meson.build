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

    'src/adt/zstring_view.cpp',

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

catch2 = dependency('catch2-with-main')

coretest = declare_dependency(
    include_directories : '.',
    dependencies : [
        catch2,
        cthulhu.get_variable('backtrace'),
        cthulhu.get_variable('format_core'),
        cthulhu.get_variable('io'),
    ]
)

test('zstring view',
    executable('core-adt-zstring-view', 'test/adt/zstring_view.cpp',
        dependencies : [ core, coretest ]
    ),
    suite : 'core'
)

test('vector',
    executable('core-adt-vector', 'test/adt/vector.cpp',
        dependencies : [ core, coretest ]
    ),
    suite : 'core'
)

test('bitset',
    executable('core-adt-bitset', 'test/adt/bitset.cpp',
        dependencies : [ core, coretest ]
    ),
    suite : 'core'
)

test('priority queue',
    executable('core-adt-priority-queue', 'test/adt/priority_queue.cpp',
        dependencies : [ core, coretest ]
    ),
    suite : 'core'
)
