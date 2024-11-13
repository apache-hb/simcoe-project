project('compressonatorcli',
    license : 'MIT',
    license_files : [ 'license/license.txt' ],
    meson_version : '>=1.1.0',
)

cli = find_program('compressonatorcli.exe', dirs : meson.current_source_dir(), native : true)

meson.override_find_program('compressonatorcli', cli)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
