project('fmt', 'cpp',
  default_options: ['cpp_std=c++14'],
  version: '11.0.99',
  license: 'BSD',
  license_files: ['LICENSE'],
  meson_version : '>=1.1.0',
)

cpp = meson.get_compiler('cpp')

fmt_private_cpp_args = []
fmt_interface_cpp_args = cpp.get_supported_arguments('-Wno-dangling-reference')
if get_option('default_library') == 'shared'
  fmt_private_cpp_args += ['-DFMT_LIB_EXPORT']
  fmt_interface_cpp_args += ['-DFMT_SHARED']
endif

fmt_lib = library(
  'fmt',
  'src/format.cc',
  'src/os.cc',
  cpp_args: fmt_private_cpp_args,
  include_directories: 'include',
  build_by_default: false,
  install: not meson.is_subproject(),
)

install_subdir('include',
  strip_directory: true,
  install_dir: get_option('includedir'),
)

fmt_dep = declare_dependency(
  include_directories: 'include',
  compile_args: fmt_interface_cpp_args,
  link_with: fmt_lib,
)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('fmt', fmt_dep)
endif

fmt_header_only_dep = declare_dependency(
  include_directories: 'include',
  compile_args: '-DFMT_HEADER_ONLY',
)

pkg_mod = import('pkgconfig')
pkg_mod.generate(fmt_lib,
  name: 'fmt',
  filebase: 'fmt',
)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
