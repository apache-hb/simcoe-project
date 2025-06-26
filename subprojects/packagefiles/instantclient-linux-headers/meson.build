project('instantclient', 'c',
    version : '23.6.0',
    license : 'LicenseRef-Proprietary',
    license_files : [ 'instantclient_23_8/SDK_LICENSE' ],
    meson_version : '>=1.1.0'
)

c = meson.get_compiler('c')
inc = include_directories('instantclient_23_8/sdk/include')

instantclient_dep = declare_dependency(
    include_directories : inc,
)

meson.override_dependency('instantclient-linux-headers', instantclient_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
