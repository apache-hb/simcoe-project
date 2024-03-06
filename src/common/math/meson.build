math_include = include_directories('include')

libmath = library('math', 'src/math.cpp',
    include_directories : math_include,
    dependencies : core
)

math = declare_dependency(
    link_with : libmath,
    include_directories : math_include,
    dependencies : core
)
