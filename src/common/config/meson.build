config_include = include_directories('include')

###
### reflection data
###

src = [
    metacc.process('include/config/config.hpp'),
]

deps = [ core, meta ]

libconfig_meta = library('config-meta', src,
    include_directories : config_include,
    dependencies : deps,
)

config_meta = declare_dependency(
    link_with : libconfig_meta,
    include_directories : [ config_include, libconfig_meta.private_dir_include() ],
    dependencies : deps,
)

###
### public api and implentation
###

tomlplusplus = dependency('tomlplusplus')

src = [
    'src/config/options.cpp',
    'src/config/command_line.cpp',
    'src/config/config.cpp',
]

libconfig = library('config', src,
    include_directories : config_include,
    cpp_args : [ '-DSM_CONFIG_BUILD=1' ],
    cpp_pch : 'src/config/stdafx.hpp',
    dependencies : [ config_meta, tomlplusplus ],
)

config = declare_dependency(
    link_with : libconfig,
    include_directories : config_include,
    dependencies : config_meta,
)

testdeps = [ config, coretest ]

testcases = {
    'Global config variables' : 'test/options.cpp',
    'Command line parsing' : 'test/command_line.cpp',
    'Parsing config from file' : 'test/config.cpp',
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
