base_include = include_directories('include')

library('base', 'src/base.cpp', include_directories : base_include, build_by_default : false)

base = declare_dependency(
    include_directories : base_include,
)
