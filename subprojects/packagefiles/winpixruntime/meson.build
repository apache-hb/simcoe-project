project('winpixruntime', 'c',
    version : '1.0.231030001',

    license : 'UNLICENSED',
    license_files : [ 'license.txt' ]
)

host_cpu = host_machine.cpu_family()
cc = meson.get_compiler('c')

if host_cpu == 'x86_64'
    bindir = 'bin/X64'
elif host_cpu == 'aarch64'
    bindir = 'bin/ARM64'
else
    error('The winpixruntime package does not support the ' + host_cpu + ' architecture')
endif

icon = files('WinPixEventRuntimeIcon.png')

bindir = meson.current_source_dir() / bindir
winpix_event_runtime = cc.find_library(bindir / 'WinPixEventRuntime')

inc = include_directories('Include')

winpix_event_runtime_dep = declare_dependency(
    include_directories : inc,
    dependencies : winpix_event_runtime,
)

meson.override_dependency('winpix-event-runtime', winpix_event_runtime_dep)
