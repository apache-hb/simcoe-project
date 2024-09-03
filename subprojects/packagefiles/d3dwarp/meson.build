project('d3d-warp',
    version : '1.0.11',
    license : 'LicenseRef-Proprietary',
    license_files : [ 'LICENSE.txt' ],
    meson_version : '>=1.1.0'
)

host_cpu = host_machine.cpu_family()
binaries = 'build/native/bin'

if host_cpu == 'x86_64'
    bindir = binaries / 'x64'
elif host_cpu == 'x86'
    bindir = binaries / 'win32'
elif host_cpu == 'aarch64'
    bindir = binaries / 'arm64'
else
    error('No warp redist is provided for ' + host_cpu)
endif

bindir = meson.current_source_dir() / bindir
