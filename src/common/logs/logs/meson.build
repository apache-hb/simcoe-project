###
### reflection data
###

opt = get_option('logs_include_source_info').disable_auto_if(is_release)

log_cdata = configuration_data()
log_cdata.set10('SMC_LOGS_INCLUDE_SOURCE_INFO', opt.allowed(),
    description : 'Include source information and log message content in the game binary.'
)

log_config = configure_file(
    output : 'simcoe_logs_config.h',
    configuration : log_cdata
)

logs_include = include_directories('include', '.')

src = [
    metacc.process('include/logs/logs.hpp'),
    log_config,
]

deps = [ core, meta ]

liblogs_meta = library('logs-meta', src,
    include_directories : logs_include,
    dependencies : deps
)

logs_meta = declare_dependency(
    link_with : liblogs_meta,
    include_directories : [ logs_include, liblogs_meta.private_dir_include() ],
    dependencies : deps
)

###
### public api and implementation
###

src = [
    'src/logging.cpp',
    'src/logger.cpp',

    'src/timer.cpp',
]

deps = [ core, logs_meta ]

liblogs = library('logs', src,
    cpp_args : [ '-DSM_LOGS_BUILD=1' ],
    cpp_pch : 'src/stdafx.hpp',
    include_directories : logs_include,
    dependencies : deps
)

logs = declare_dependency(
    link_with : liblogs,
    include_directories : logs_include,
    dependencies : deps
)
