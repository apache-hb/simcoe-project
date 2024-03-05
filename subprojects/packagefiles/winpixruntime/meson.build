project('winpixruntime', 'c',
    version : '1.0.231030001',

    license : 'UNLICENSED',
    license_files : [ 'license.txt' ]
)

host_cpu = host_machine.cpu_family()
cc = meson.get_compiler('c')

if host_cpu == 'x86_64'
    bindir = 'bin/x64'
elif host_cpu == 'aarch64'
    bindir = 'bin/arm64'
else
    error('The winpixruntime package does not support the ' + host_cpu + ' architecture')
endif

icon = files('WinPixEventRuntimeIcon.png')

redist = files(bindir / 'WinPixEventRuntime.dll')
winpixruntime = cc.find_library('WinPixEventRuntime', dirs : bindir)

inc = include_directories('Include/WinPixEventRuntime')

winpixruntime_dep = declare_dependency(
    include_directories : inc,
    link_with : winpixruntime,
)

meson.override_dependency('winpixruntime', winpixruntime_dep)
