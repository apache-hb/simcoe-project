project('instantclient', 'c')

c = meson.get_compiler('c')
inc = include_directories('sdk/include')
lib = c.find_library('oci', dirs : meson.current_source_dir() / 'sdk/lib/msvc')

instantclient_dep = declare_dependency(
    include_directories : inc,
    dependencies : lib
)

meson.override_dependency('instantclient-windows', instantclient_dep)
