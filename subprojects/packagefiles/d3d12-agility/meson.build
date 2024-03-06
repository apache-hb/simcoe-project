project('d3d12-agility', 'cpp',
    version : '1.611.2',
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
binaries = 'build/native/bin'

if host_cpu == 'x86_64'
    bindir = binaries / 'x64'
elif host_cpu == 'x86'
    bindir = binaries / 'win32'
elif host_cpu == 'arm'
    bindir = binaries / 'arm'
elif host_cpu == 'aarch64'
    bindir = binaries / 'arm64'
else
    error('The agility sdk does not provide binaries for ' + host_cpu)
endif

# name the icon asset for people who want to use it
icon = files('icon.png')

# override the system d3dconfig with the new one
d3dconfig = find_program(bindir / 'd3dconfig')

bindir = meson.current_source_dir() / bindir

# we only need to export the include directory for d3d12
d3d12_include = include_directories('build/native/include')
d3d12 = declare_dependency(
    dependencies : [ cpp.find_library('d3d12'), cpp.find_library('dxgi'), cpp.find_library('dxguid') ],
    include_directories : d3d12_include
)

# export everything
meson.override_find_program('d3dconfig', d3dconfig)
meson.override_dependency('d3d12', d3d12)
