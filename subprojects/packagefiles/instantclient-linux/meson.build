project('instantclient', 'c',
    version : '23.6.0',
    license : 'LicenseRef-Proprietary',
    license_files : [ 'SDK_LICENSE' ],
    meson_version : '>=1.1.0'
)

c = meson.get_compiler('c')
inc = include_directories('sdk/include')
lib = c.find_library('oci', dirs : meson.current_source_dir() / 'sdk/lib/msvc')

instantclient_dep = declare_dependency(
    include_directories : inc,
    dependencies : lib
)

meson.override_dependency('instantclient-linux', instantclient_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
