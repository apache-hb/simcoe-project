input_include = include_directories('.', 'include')

src = [
    'src/input.cpp',
    'src/utils.cpp',
]

deps = [ core, logs, math ]

libinput = library('input', src,
    cpp_args : [ '-DSM_INPUT_BUILD=1' ],
    include_directories : [ input_include ],
    dependencies : deps
)

input = declare_dependency(
    link_with : libinput,
    include_directories : [ input_include ],
    dependencies : deps
)
