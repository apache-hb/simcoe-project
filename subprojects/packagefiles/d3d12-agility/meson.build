project('d3d12-agility', 'cpp',
    version : '1.611.2',
    # the binary data is licensed under a microsoft private license
    # the code is licensed under the MIT license
    license : 'LicenseRef-Proprietary AND MIT',
    license_files : [ 'LICENSE.txt', 'LICENSE-CODE.txt' ],
    meson_version : '>=1.1.0',
)

cpu = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')
root = meson.current_source_dir() / 'build/native'
os = host_machine.system()

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
d3dconfig = find_program(bindir / 'd3dconfig.exe')

d3d12core = files(bindir / 'D3D12Core.dll')
d3d12sdklayers = files(bindir / 'd3d12SDKLayers.dll')

d3d12_dep = declare_dependency(
    dependencies : [
        cpp.find_library('d3d12'),
        cpp.find_library('dxgi'),
        cpp.find_library('dxguid'),
    ],
    variables : {
        'd3d12core': meson.current_source_dir() / bindir / 'D3D12Core.dll',
        'd3d12sdklayers': meson.current_source_dir() / bindir / 'd3d12SDKLayers.dll',
        'bindir' : bindir,
    }
)

meson.override_find_program('d3dconfig', d3dconfig)
meson.override_dependency('d3d12', d3d12_dep)

install_data(bindir / 'D3D12Core.dll',
    install_dir : get_option('bindir') / 'redist/d3d12',
    install_tag : 'bin',
)

install_data('LICENSE.txt',
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
