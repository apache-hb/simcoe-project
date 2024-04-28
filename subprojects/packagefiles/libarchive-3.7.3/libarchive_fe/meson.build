libarchive_fe = static_library(
  'libarchive_fe',
  [
    'err.c',
    'line_reader.c',
    'passphrase.c',
  ],
  dependencies: libarchive_dep,
  override_options: [ 'unity=off' ]
)

libarchive_fe_dep = declare_dependency(
  dependencies: libarchive_dep,
  include_directories: include_directories('.'),
  link_with: libarchive_fe,
)
