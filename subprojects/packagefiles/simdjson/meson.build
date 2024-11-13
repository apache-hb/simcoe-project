project(
  'simdjson',
  'cpp',
  license: 'Apache-2.0',
  version: '3.9.0',
  license_files: ['LICENSE'],
  meson_version : '>=1.1.0',
)

simdjson_inc = include_directories('singleheader')

cpp = meson.get_compiler('cpp')

simdjson_args_public = cpp.get_supported_arguments([
  '-Wno-deprecated-literal-operator'
])
simdjson_args_private = []
if cpp.get_id() == 'msvc'
  simdjson_args_public += '-DSIMDJSON_VISUAL_STUDIO'
  simdjson_args_private += [ '-DSIMDJSON_VISUAL_STUDIO', '-DSIMDJSON_BUILDING_WINDOWS_DYNAMIC_LIBRARY' ]
endif

simdjson_lib = library(
  'simdjson',
  sources: files('singleheader/simdjson.cpp'),
  include_directories: simdjson_inc,
  cpp_args: simdjson_args_private,
  install: true,
  # see #1218, simdjson does not link properly with this enabled.
  # gnu_symbol_visibility: 'hidden'
)

install_headers('singleheader/simdjson.h')

simdjson_dep = declare_dependency(
  include_directories: simdjson_inc,
  link_with: simdjson_lib,
  compile_args: simdjson_args_public,
)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
