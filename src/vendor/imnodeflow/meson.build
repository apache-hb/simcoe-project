src = [ 'src/ImNodeFlow.cpp' ]
inc = include_directories('include')

libimnodeflow = library('imnodeflow', src,
    cpp_args : [ '-DIMGUI_DEFINE_MATH_OPERATORS=1' ],
    include_directories : inc,
    dependencies : imgui
)

imnodeflow = declare_dependency(
    include_directories : inc,
    link_with : libimnodeflow,
    dependencies : imgui
)
