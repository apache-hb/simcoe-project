src = [
    'src/main.cpp',
    daocc.process('data/vic20.xml'),
]

deps = [
    core, system, log_sinks, threads,
    imgui, render_core, input,
    archive, config, draw,
    tracy, launch, launch_gui, filebrowser,
]

executable('client', src,
    cpp_pch : 'src/stdafx.hpp',
    install : true,
    win_subsystem : get_option('client_win_subsystem'),
    link_depends : [ manifest, res ],
    link_args : [
        # embed the resource file
        res[0].full_path(),

        # embed the manifest file
        '/manifest:embed', '/manifestinput:' + manifest,
    ],
    dependencies : deps
)
