project('d3d-warp',
    version : '1.0.11',
    license : 'LicenseRef-Proprietary',
    license_files : [ 'LICENSE.txt' ],
    meson_version : '>=1.1.0'
)

host_cpu = host_machine.cpu_family()
binaries = 'build/native'

if host_cpu == 'x86_64'
    bindir = binaries / 'amd64'
elif host_cpu == 'x86'
    bindir = binaries / 'x86'
else
    error('No warp redist is provided for ' + host_cpu)
endif

bindir = meson.current_source_dir() / bindir
