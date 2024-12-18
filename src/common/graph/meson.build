###
### framegraph api
###

graph_include = include_directories('.', 'include')

src = [
    'src/graph.cpp',
    'src/builder.cpp',
    'src/context.cpp',
]

deps = [ render ]

libgraph = library('graph', src,
    cpp_pch : 'src/stdafx.hpp',
    include_directories : [ graph_include ],
    dependencies : deps
)

graph = declare_dependency(
    link_with : libgraph,
    include_directories : [ graph_include ],
    dependencies : deps
)

###
### tests
###

testcases = {
    'Graph setup': 'test/create.cpp',
}

foreach name, source : testcases
    exe = executable('test-graph-' + name.to_lower().replace(' ', '-'), source,
        dependencies : [ rendertest, graph ]
    )

    test(name, exe,
        suite : 'graph',
        kwargs : testkwargs
    )
endforeach
