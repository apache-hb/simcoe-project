project('dstorage', 'cpp',
    version : '1.2.2',
    # the binary data is licensed under a microsoft private license
    # the code is licensed under the MIT license
    license : 'LicenseRef-Proprietary AND MIT',
    license_files : [ 'LICENSE.txt', 'LICENSE-CODE.txt' ],
    meson_version : '>=1.1.0',
)

host_cpu = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')
root = meson.current_source_dir() / 'native'

if host_cpu == 'x86_64'
    archdir = 'x64'
elif host_cpu == 'x86'
    archdir = 'x86'
elif host_cpu == 'aarch64'
    archdir = 'ARM64'
else
    error('The dstorage sdk does not provide binaries for ' + host_cpu)
endif

bindir = root / 'bin' / archdir
libdir = root / 'lib' / archdir

icon = files('icon.png')

# specify the full path rather than providing the libs kwarg
# as the system may have a different version of the library
lib = cpp.find_library(libdir / 'dstorage')
inc = include_directories('native/include')

dstorage_dep = declare_dependency(
    dependencies : lib,
    include_directories : inc,
    variables : {
        'bindir' : bindir,
        'libdir' : libdir,
    }
)

meson.override_dependency('dstorage', dstorage_dep)

install_data(bindir / 'dstorage.dll', bindir / 'dstoragecore.dll',
    install_dir : get_option('bindir') / 'redist',
    install_tag : 'runtime',
)

install_data('LICENSE.txt',
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
