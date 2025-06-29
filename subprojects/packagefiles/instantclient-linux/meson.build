project('instantclient', 'c',
    version : '23.6.0',
    license : 'LicenseRef-Proprietary',
    license_files : [ 'instantclient_23_8/BASIC_LITE_LICENSE' ],
    meson_version : '>=1.1.0'
)

c = meson.get_compiler('c')

instantclient_dep = declare_dependency(
    link_args : [
        '-L' + meson.current_source_dir() / 'instantclient_23_8',
        '-Wl,-Bdynamic,-l:libclntsh.so.23.1',
        '-Wl,-Bdynamic,-l:libclntshcore.so.23.1',
    ]
)

meson.override_dependency('instantclient-linux', instantclient_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
