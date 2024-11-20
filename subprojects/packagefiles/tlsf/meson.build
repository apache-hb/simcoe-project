project('tlsf', 'c')

cc = meson.get_compiler('c')
args = cc.get_supported_arguments([ '-Wno-format-pedantic' ])

libtlsf = static_library('tlsf', 'tlsf.c',
    c_args: args,
    include_directories: '.'
)

tlsf_dep = declare_dependency(
    link_with: libtlsf,
    include_directories: '.'
)

meson.override_dependency('tlsf', tlsf_dep)
