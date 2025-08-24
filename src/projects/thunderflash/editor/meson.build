src = [
    'src/main.cpp',
    daocc.process('data/vic20.xml'),
]

deps = [
    core, system, log_appenders, threads,
    imgui, input,
    archive, config, draw,
    tracy, launch, launch_gui, filebrowser,
    dependency('vulkan'),
]

executable('editor', src,
    cpp_pch : 'src/stdafx.hpp',
    install : true,
    win_subsystem : get_option('editor_win_subsystem'),
    link_depends : [ manifest, res ],
    link_args : [
        # embed the resource file
        res[0].full_path(),

        # embed the manifest file
        '/manifest:embed', '/manifestinput:' + manifest,
    ],
    dependencies : deps
)

install_data('data/vic20-uk.bin',
    install_dir : get_option('bindir'),
    install_tag : 'bin'
)
