project('dxc', 'c',
    version : '1.8.2405',
    # dxil.dll is licensed under the MS license
    # d3d12shader.h is licensed under the MIT license
    # all other files are licensed under the LLVM license
    license : 'LicenseRef-Proprietary AND MIT AND LLVM',
    license_files : [ 'LICENSE-MS.txt', 'LICENSE-LLVM.txt' ],
    meson_version : '>=1.1.0',
)

host_cpu = host_machine.cpu_family()
cc = meson.get_compiler('c')
root = meson.current_source_dir()

if host_cpu != 'x86_64'
    error('No prebuilt dxc binaries for ' + host_cpu)
endif

bindir = root / 'bin'
libdir = 'lib'

# specify the full path rather than providing the libs kwarg
# as the system may have a different version of dxc or dxcompiler installed
dxc = find_program(bindir / 'dxc')
dxcompiler = cc.find_library(root / libdir / 'libdxcompiler')
dxil = cc.find_library(root / libdir / 'libdxil')
inc = include_directories('include/dxc')

dxcompiler_dep = declare_dependency(
    dependencies : dxcompiler,
    include_directories : inc,
)

meson.override_find_program('dxc-linux', dxc)
meson.override_dependency('dxcompiler-linux', dxcompiler_dep)

install_data([ libdir / 'libdxil.so', libdir / 'libdxcompiler.so' ],
    install_dir : get_option('libdir')
)

install_data([ 'LICENSE-MS.txt', 'LICENSE-LLVM.txt' ],
    install_dir : get_option('licensedir'),
    rename : [
        meson.project_name() + '.LICENSE-MS.txt',
        meson.project_name() + '.LICENSE-LLVM.txt'
    ],
    install_tag : 'legal',
)
