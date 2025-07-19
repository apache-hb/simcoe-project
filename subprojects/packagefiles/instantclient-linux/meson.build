project('instantclient', 'c',
    version : '23.6.0',
    license : 'LicenseRef-Proprietary',
    license_files : [ 'instantclient_23_8/BASIC_LITE_LICENSE' ],
    meson_version : '>=1.1.0'
)

cc = meson.get_compiler('cpp')
fixup_symlinks = find_program('fixup_symlinks.py')

here = meson.current_source_dir() / 'instantclient_23_8'

libclntsh_name = run_command(fixup_symlinks, here / 'libclntsh.so', here / 'libclntsh.so.23.1',
    check : true,
    capture : true
)

libclntshcore_name = run_command(fixup_symlinks, here / 'libclntshcore.so', here / 'libclntshcore.so.23.1',
    check : true,
    capture : true
)

libclntsh = cc.find_library(libclntsh_name.stdout(), dirs : [here])
libclntshcore = cc.find_library(libclntshcore_name.stdout(), dirs : [here])
libnnz = cc.find_library('nnz', dirs : [here])

instantclient_dep = declare_dependency(
    dependencies : [ libclntsh, libclntshcore, libnnz ],
)

meson.override_dependency('instantclient-linux', instantclient_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
