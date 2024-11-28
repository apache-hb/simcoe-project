# ags provides headers that try to be compatible with C
# but they are not, so we need to use cpp instead of c
# there is an open pr to resolve this https://github.com/GPUOpen-LibrariesAndSDKs/AGS_SDK/pull/51
project('AGS', 'cpp',
    version : '6.2.0',
    # all headers and binaries are licensed as all rights reserved by AMD
    license : 'LicenseRef-Proprietary',
    license_files : [ 'LICENSE.txt' ],
    meson_version : '>=1.1.0'
)

cpp = meson.get_compiler('cpp')
cpu = host_machine.cpu_family()
root = meson.current_source_dir() / 'ags_lib'
libdir = root / 'lib'

if cpu == 'x86_64'
    lib = cpp.find_library(libdir / 'amd_ags_x64')
    dll = files(libdir / 'amd_ags_x64.dll')
elif cpu == 'x86'
    lib = cpp.find_library(libdir / 'amd_ags_x86')
    dll = files(libdir / 'amd_ags_x86.dll')
else
    error('amd_ags is not available on ' + cpu)
endif

ags_dep = declare_dependency(
    include_directories : 'ags_lib/inc',
    dependencies : lib
)

meson.override_dependency('amd_ags', ags_dep)

install_data(dll,
    install_dir : get_option('bindir') / 'redist',
    install_tag : 'runtime'
)

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
