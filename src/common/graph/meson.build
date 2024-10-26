###
### reflection data
###

graph_include = include_directories('.', 'include')

deps = [ render_next ]

src = [
    metacc.process('include/graph/graph.hpp'),
]

libgraph_meta = library('graph-meta', src,
    include_directories : [ graph_include, reflect_include ],
    dependencies : deps
)

graph_meta = declare_dependency(
    link_with : libgraph_meta,
    include_directories : [ graph_include, libgraph_meta.private_dir_include() ],
    dependencies : deps
)

###
### framegraph api
###

src = [
    'src/graph.cpp',
]

deps = [ graph_meta ]

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
