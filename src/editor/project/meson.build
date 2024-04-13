src = [
    'src/cube.cpp',

    meta_tool.process('include/project/cube.hpp', extra_args : [ 'project' ])
]

inc = include_directories('include')

deps = [ game ]

libproject = library('project', src,
    include_directories : [ inc, 'include/project' ],
    dependencies : deps,
    cpp_pch : 'src/stdafx.hpp'
)

project = declare_dependency(
    link_with : libproject,
    include_directories : [ inc, libproject.private_dir_include() ]
)
