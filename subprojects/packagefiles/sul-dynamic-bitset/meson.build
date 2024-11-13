project('dynamic_bitset', 'cpp',
    license : 'MIT',
    license_files : [ 'LICENSE' ],
    meson_version : '>=1.1.0',
)

dynamic_bitset_dep = declare_dependency(
    include_directories : 'include'
)

meson.override_dependency('dynamic_bitset', dynamic_bitset_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
