input_include = include_directories('.', 'include')

src = [
    'src/input.cpp',
    'src/utils.cpp',
    reflect.process('data/input.ct')
]

deps = [ core, logs, math ]

libinput = library('input', src,
    cpp_args : [ '-DSM_INPUT_BUILD=1' ],
    include_directories : [ input_include, reflect_include ],
    dependencies : deps
)

input = declare_dependency(
    link_with : libinput,
    include_directories : [ input_include, reflect_include, libinput.private_dir_include() ],
    dependencies : deps
)
