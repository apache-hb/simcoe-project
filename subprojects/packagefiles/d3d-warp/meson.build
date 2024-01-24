project('d3d_warp',
    version : '1.0.9',
    # the binary is licensed under a microsoft license
    # do not redistribute the binary
    license : 'UNLICENSED',
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

redist = files(bindir / 'd3d10warp.dll')
redist_pdb = files(bindir / 'd3d10warp.pdb')
