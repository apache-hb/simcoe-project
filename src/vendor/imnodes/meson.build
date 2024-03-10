src = [ 'ImNodes.cpp', 'ImNodesEz.cpp' ]

libimnodes = library('imnodes', src,
    dependencies : imgui
)

imnodes = declare_dependency(
    include_directories : '.',
    link_with : libimnodes,
    dependencies : imgui
)
