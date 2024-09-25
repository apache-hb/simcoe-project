project('instantclient', 'c')

c = meson.get_compiler('c')
inc = include_directories('instantclient23_5/sdk/include')
lib = c.find_library('oci', dirs : meson.current_source_dir() / 'instantclient23_5/sdk/lib/msvc')

instantclient_dep = declare_dependency(
    include_directories : inc,
    dependencies : lib
)

meson.override_dependency('instantclient', instantclient_dep)
