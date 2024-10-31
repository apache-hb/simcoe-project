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

    'src/allocators/bitmap.cpp',
    'src/allocators/slab.cpp',

    'src/error/error.cpp',
]

if is_windows
    src += [ 'src/windows/console.cpp', 'src/windows/core.cpp' ]
else
    src += [ 'src/linux/console.cpp', 'src/linux/core.cpp' ]
endif

inc = include_directories('.', 'include', 'src')

libcore = library('core', src, simcoe_config,
    cpp_args : [ '-DSM_CORE_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ inc, core_include, config_include, reflect_include ],
    dependencies : core_dependencies
)

core = declare_dependency(
    link_with : libcore,
    include_directories : [ core_include, config_include ],
    dependencies : core_dependencies
)

catch2 = dependency('catch2-with-main')
testargs = [ '--reporter=TAP', '--allow-running-no-tests' ]
testkwargs = { 'protocol' : 'tap', 'args' : testargs, 'timeout': 5 }
benchkwargs = { 'timeout': 600 }

deps = [ core, catch2 ]

libtest = library('test-core', 'test/common.cpp',
    include_directories : [ core_include ],
    dependencies : [
        deps,
        cthulhu.get_variable('backtrace'),
        cthulhu.get_variable('format_core'),
        cthulhu.get_variable('io'),
    ]
)

coretest = declare_dependency(
    include_directories : '.',
    link_with : libtest,
    dependencies : deps
)

testcases = {
    'String' : 'test/string.cpp',
    'Null terminated string view' : 'test/adt/zstring_view.cpp',
    'Vector' : 'test/adt/vector.cpp',
    'Small vector': 'test/adt/small_vector.cpp',
    'Bit set' : 'test/adt/bitset.cpp',
    'Priority queue' : 'test/adt/priority_queue.cpp',
    'Symbol names' : 'test/symbols.cpp',
    'Sync primitives' : 'test/sync.cpp',
}

foreach name, source : testcases
    exe = executable('test-core-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ core, coretest ]
    )

    test(name, exe,
        suite : 'core',
        kwargs : testkwargs
    )
endforeach
