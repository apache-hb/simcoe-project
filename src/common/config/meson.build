inc = include_directories('include')
tomlplusplus = dependency('tomlplusplus')

deps = [ core, meta ]

src = [
    'src/options.cpp',
    'src/command_line.cpp',
    'src/config.cpp',

    metacc.process('include/config/option.hpp')
]

libconfig = library('config', src,
    include_directories : inc,
    cpp_args : [ '-DSM_CONFIG_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    dependencies : [ deps, logs, tomlplusplus ],
    override_options : [ 'unity=off' ]
)

config = declare_dependency(
    link_with : libconfig,
    include_directories : [ inc, libconfig.private_dir_include() ],
    dependencies : deps
)

testdeps = [ config, coretest ]

testcases = {
    'cvar' : 'test/options.cpp',
    'command line parsing' : 'test/command_line.cpp',
    'loading config from file' : 'test/config.cpp',
}

foreach name, source : testcases
    exe = executable('test-config-' + name.to_lower().replace(' ', '-'), source,
        dependencies : testdeps
    )

    test(name, exe,
        suite : 'config',
        kwargs : testkwargs
    )
endforeach
