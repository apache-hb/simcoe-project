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

# get the redist dlls
# meson currently has no mechanism for installing redist files
# so we just export them as files and the user can `install_data` them
redist = files(bindir / 'D3D12Core.dll', bindir / 'D3D12SDKLayers.dll')
redist_pdb = files(bindir / 'D3D12Core.pdb', bindir / 'd3d12SDKLayers.pdb')

# we only need to export the include directory for d3d12
d3d12_include = include_directories('build/native/include')
d3d12 = declare_dependency(
    dependencies : [ cpp.find_library('d3d12'), cpp.find_library('dxgi'), cpp.find_library('dxguid') ],
    include_directories : d3d12_include
)

# build d3dx12
d3dx12_include = include_directories('build/native/include/d3dx12')
d3dx12_src = 'build/native/src/d3dx12/d3dx12_property_format_table.cpp'

needs_fixup = get_option('cpp_eh') == 'none'

if get_option('cpp_eh') == 'none'
    awful = generator(find_program('awful.py'),
        output : '@BASENAME@.cpp',
        arguments : [ '@INPUT@', '@OUTPUT@' ]
    )
    src = awful.process(d3dx12_src)
else
    src = d3dx12_src
endif

libd3dx12 = library('d3dx12', src,
    cpp_args : [ '-D_HAS_EXCEPTIONS=0' ],
    dependencies : d3d12,
    include_directories : d3dx12_include
)

d3dx12 = declare_dependency(
    dependencies : d3d12,
    include_directories : d3dx12_include,
    link_with : libd3dx12
)

# export everything
meson.override_find_program('d3dconfig', d3dconfig)
meson.override_dependency('d3d12', d3d12)
meson.override_dependency('d3dx12', d3dx12)
