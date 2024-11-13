project('wil', 'cpp',
    version : '1.0.240803.1',
    license : 'MIT',
    license_files : [ 'LICENSE' ],
    meson_version : '>=1.1.0',
)

inc = include_directories('include')
wil = declare_dependency(include_directories : inc)
meson.override_dependency('wil', wil)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
