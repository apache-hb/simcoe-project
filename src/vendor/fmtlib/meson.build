fmt = subproject('fmt').get_variable('fmt_header_only_dep')

fmtlib = declare_dependency(
    include_directories : 'include',
    dependencies : [ cthulhu.get_variable('base'), fmt ]
)
