project('simcoe', 'cpp',
    version : '0.0.1',
    default_options : [
        'cpp_std=c++20',

        'cpp_rtti=false',
        'cpp_eh=none',

        'warning_level=3',
        'werror=true'
    ]
)

add_project_arguments('-D_HAS_EXCEPTIONS=0', language : 'cpp')
add_project_arguments('/wd4251', language : 'cpp')

subdir('common/core')
subdir('common/service')

executable('client', 'client/src/main.cpp',
    dependencies : [ simcoe_core, simcoe_service ]
)
