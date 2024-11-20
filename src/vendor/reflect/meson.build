lex = cthulhu.get_variable('lex')
parse = cthulhu.get_variable('parse')

src = [
    'src/scan.c',
    'src/ast.c',
    'src/eval.c',
    'src/events.c',
    'src/emit.c',
    'src/sema.c',
    'src/sema.cpp',
    lex.process('src/ref.l'),
    parse.process('src/ref.y')
]

deps = [
    cthulhu.get_variable('interop'),
    cthulhu.get_variable('gmp'),
    cthulhu.get_variable('scan'),
    cthulhu.get_variable('notify'),
    cthulhu.get_variable('events'),
    cthulhu.get_variable('std'),
    cthulhu.get_variable('memory'),
    cthulhu.get_variable('io'),
    cthulhu.get_variable('tree'),
]

library_deps = [
    cthulhu.get_variable('util')
]

cli_deps = [
    cthulhu.get_variable('setup'),
    cthulhu.get_variable('config'),
    cthulhu.get_variable('argparse'),
    cthulhu.get_variable('format'),
]

reflect_args = cpp.get_supported_arguments(
    '/wd4201', '/wd4005',
    '-Wno-format-security', '-Wno-missing-designated-field-initializers',
    '-Wno-nested-anon-types', '-Wno-gnu-anonymous-struct'
)

reflect_args += cthulhu.get_variable('generated_args')

ref = library('ref', src,
    dependencies : deps + library_deps,
    c_args : reflect_args,
    cpp_args : reflect_args,
    include_directories : [ 'src', 'include' ],
    override_options : [ 'unity=off' ]
)

reflect_cli = executable('reflect_cli', 'src/main.cpp',
    dependencies : deps + cli_deps,
    link_with : ref,
    include_directories : [ 'include', ref.private_dir_include() ],
    cpp_args : reflect_args,
    c_args : reflect_args,
    override_options : [ 'unity=off', 'b_sanitize=none' ],
    native : true
)

reflect = generator(reflect_cli,
    output : [ '@BASENAME@.reflect.h' ],
    arguments : [ '/header:@OUTPUT0@', '@INPUT@' ]
)

reflect_include = include_directories('data/include')
