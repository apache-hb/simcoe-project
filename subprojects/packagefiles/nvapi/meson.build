project('nvapi', 'c',
    version : '550.0.0',
)

host_cpu = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')

bindir = meson.current_source_dir()

if host_cpu == 'x86_64'
    bindir = bindir / 'amd64'
elif host_cpu == 'x86'
    bindir = bindir / 'x86'
else
    error('NVAPI is not supported on ' + host_cpu)
endif
