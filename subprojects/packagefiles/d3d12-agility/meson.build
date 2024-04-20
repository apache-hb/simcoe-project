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

cpu = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')
root = meson.current_source_dir() / 'build/native'

if cpu == 'x86_64'
    archdir = 'x64'
elif cpu == 'x86'
    archdir = 'win32'
elif cpu == 'aarch64'
    archdir = 'arm64'
else
    error('The agility sdk is not available on ' + cpu)
endif

bindir = root / 'bin' / archdir

# name the icon asset for people who want to use it
icon = files('icon.png')

# use an absolute path to d3dconfig to avoid finding
# the system d3dconfig
d3dconfig = find_program(bindir / 'd3dconfig')

d3d12 = declare_dependency(
    dependencies : [ cpp.find_library('d3d12'), cpp.find_library('dxgi'), cpp.find_library('dxguid') ]
)

meson.override_find_program('d3dconfig', d3dconfig)
meson.override_dependency('d3d12', d3d12)
