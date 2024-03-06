project('dstorage', 'cpp',
    version : '1.2.2',
    # the binary data is licensed under a microsoft private license
    # the code is licensed under the MIT license
    # !!! unlicensed DOES NOT mean public domain, redistribute this stuff
    # !!! and microsoft lawyers will have your head
    license : 'UNLICENSED AND MIT',
    license_files : [ 'LICENSE.txt', 'LICENSE-CODE.txt' ],
    meson_version : '>=1.1.0'
)

host_cpu = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')

if host_cpu == 'x86_64'
    bindir = 'native/bin/x64'
    libdir = 'native/lib/x64'
elif host_cpu == 'x86'
    bindir = 'native/bin/x86'
    libdir = 'native/lib/x86'
elif host_cpu == 'aarch64'
    bindir = 'native/bin/ARM64'
    libdir = 'native/lib/ARM64'
else
    error('The dstorage sdk does not provide binaries for ' + host_cpu)
endif

bindir = meson.current_source_dir() / bindir
libdir = meson.current_source_dir() / libdir

icon = files('icon.png')

lib = cpp.find_library('dstorage', dirs : libdir)
inc = include_directories('native/include')

dstorage_dep = declare_dependency(
    dependencies : lib,
    include_directories : inc
)

meson.override_dependency('dstorage', dstorage_dep)
