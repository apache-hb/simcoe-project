project('tlsf', 'c')

libtlsf = static_library('tlsf', 'tlsf.c', include_directories: '.')

tlsf_dep = declare_dependency(
    link_with: libtlsf,
    include_directories: '.'
)

meson.override_dependency('tlsf', tlsf_dep)
