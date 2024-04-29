project('artery-font-format', 'cpp',
    license : 'MIT',
    license_files : [ 'LICENSE.txt' ]
)

artery_dep = declare_dependency(include_directories : '.')

meson.override_dependency('artery-font', artery_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('datadir') / 'licenses',
    rename : meson.project_name() + '.LICENSE.txt'
)
