src = [ 'src/info.cpp' ]

libmeta = library('meta', src,
    cpp_args : cpp.get_supported_arguments('-Wno-c99-designator'),
    include_directories : 'include'
)

meta = declare_dependency(
    link_with : libmeta,
    include_directories : 'include'
)

subdir('tool')
