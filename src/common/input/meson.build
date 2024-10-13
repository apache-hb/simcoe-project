input_include = include_directories('.', 'include')

src = [
    metacc.process('include/input/input.hpp'),
]

deps = [ core, logs, math ]

libinput_meta = library('input-meta', src,
    include_directories : [ input_include, reflect_include ],
    dependencies : deps
)

input_meta = declare_dependency(
    link_with : libinput_meta,
    include_directories : [ input_include, reflect_include, libinput_meta.private_dir_include() ],
    dependencies : deps
)

src = [
    'src/input.cpp',
    'src/utils.cpp',
]

deps = [ core, logs, math, input_meta ]

libinput = library('input', src,
    cpp_args : [ '-DSM_INPUT_BUILD=1' ],
    include_directories : [ input_include, reflect_include ],
    dependencies : deps
)

input = declare_dependency(
    link_with : libinput,
    include_directories : [ input_include, reflect_include ],
    dependencies : deps
)
