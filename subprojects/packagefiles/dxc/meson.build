project('dxc', 'cpp',
    version : '1.7.2308',
    # dxil.dll is licensed under the MS license
    # d3d12shader.h is licensed under the MIT license
    # all other files are licensed under the LLVM license
    # !!! unlicensed DOES NOT mean public domain, redistribute this stuff
    # !!! and microsoft lawyers will have your head
    license : 'UNLICENSED AND MIT AND LLVM',
    license_files : [ 'LICENSE-MS.txt', 'LICENCE-MIT.txt', 'LICENSE-LLVM.txt' ],
    meson_version : '>=1.1.0'
)

host_cpu = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')
binaries = 'build/native/bin'

if host_cpu == 'x86_64'
    bindir = binaries / 'x64'
elif host_cpu == 'x86'
    bindir = binaries / 'win32'
elif host_cpu == 'aarch64'
    bindir = binaries / 'arm64'
else
    error('No prebuilt dxc binaries for ' + host_cpu)
endif

dxc = find_program(bindir / 'dxc.exe')
meson.override_find_program('dxc', dxc)
