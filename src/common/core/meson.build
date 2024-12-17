core_include = include_directories('.', 'include')

deps = [
    base,
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

    'src/uuid.cpp',
    'src/random.cpp',

    'src/cpuid.cpp',

    'src/adt/zstring_view.cpp',

    'src/allocators/bitmap.cpp',
    'src/allocators/slab.cpp',

    'src/error/error.cpp',

    'src/text/encoding.cpp',
]

if is_windows
    src += [ 'src/windows/console.cpp', 'src/windows/core.cpp' ]
else
    src += [ 'src/linux/console.cpp', 'src/linux/core.cpp' ]
endif

inc = include_directories('.', 'include', 'src')

args = cpp.get_supported_arguments('-Wno-dangling-reference', '-Wno-c99-designator')

libcore = library('core', src, simcoe_config,
    cpp_args : [ '-DSM_CORE_BUILD=1', args ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ inc, core_include, config_include, 'src' ],
    dependencies : deps
)

core = declare_dependency(
    link_with : libcore,
    include_directories : [ core_include, config_include ],
    dependencies : deps,
    compile_args : args
)

gtest = dependency('gtest_main')
catch2 = dependency('catch2-with-main')
testargs = [ '--reporter=TAP', '--allow-running-no-tests' ]
testkwargs = { 'protocol' : 'tap', 'args' : testargs, 'timeout': 5 }
gtestkwargs = { 'protocol' : 'gtest', 'args': [ '--gtest_stack_trace_depth=64' ], 'timeout': 5 }
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

coregtest = declare_dependency(
    include_directories : '.',
    dependencies : [ core, gtest ]
)

testcases = {
    'Vector' : 'test/adt/vector.cpp',
    'Null terminated string view' : 'test/adt/zstring_view.cpp',
    'String' : 'test/string.cpp',
    'Sync primitives' : 'test/sync.cpp',
    'Symbol names' : 'test/symbols.cpp',
    'Combine' : 'test/adt/combine.cpp',
    'Small vector': 'test/adt/small_vector.cpp',
    'Units': 'test/units.cpp',
    'Bit set' : 'test/adt/bitset.cpp',
    'UUID': 'test/uuid.cpp',
    'Sized Integers': 'test/digit.cpp',
}

foreach name, source : testcases
    exe = executable('test-core-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ core, coregtest ]
    )

    test(name, exe,
        suite : 'core',
        kwargs : gtestkwargs
    )
endforeach
