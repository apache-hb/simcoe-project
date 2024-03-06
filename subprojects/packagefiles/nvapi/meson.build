project('nvapi', 'c',
    version : '550.0.0',
)

host_cpu = host_machine.cpu_family()
cc = meson.get_compiler('c')

if host_cpu == 'x86_64'
    libnvapi = cc.find_library('nvapi64', dirs : meson.current_source_dir() / 'amd64')
elif host_cpu == 'x86'
    libnvapi = cc.find_library('nvapi', dirs : meson.current_source_dir() / 'x86')
else
    error('NVAPI is not supported on ' + host_cpu)
endif

inc = include_directories('.')
nvapi_dep = declare_dependency(
    include_directories : inc
)

meson.override_dependency('nvapi', nvapi_dep)
