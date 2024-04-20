project('dxc', 'c',
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
cc = meson.get_compiler('c')
root = meson.current_source_dir() / 'build/native'

if host_cpu == 'x86_64'
    archdir = 'x64'
elif host_cpu == 'x86'
    archdir = 'win32'
elif host_cpu == 'aarch64'
    archdir = 'arm64'
else
    error('No prebuilt dxc binaries for ' + host_cpu)
endif

bindir = root / 'bin' / archdir
libdir = 'lib' / archdir

# specify the full path rather than providing the libs kwarg
# as the system may have a different version of dxc or dxcompiler installed
dxc = find_program(bindir / 'dxc.exe')
dxcompiler = cc.find_library(root / libdir / 'dxcompiler')
inc = include_directories('build/native/include')

dxcompiler_dep = declare_dependency(
    dependencies : dxcompiler,
    include_directories : inc,
)

meson.override_find_program('dxc', dxc)
meson.override_dependency('dxcompiler', dxcompiler_dep)
