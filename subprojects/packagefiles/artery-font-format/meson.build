project('artery-font-format', 'cpp',
    license : 'MIT',
    license_files : [ 'LICENSE.txt' ],
    meson_version : '>=1.1.0',
)

artery_dep = declare_dependency(include_directories : '.')

meson.override_dependency('artery-font', artery_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
