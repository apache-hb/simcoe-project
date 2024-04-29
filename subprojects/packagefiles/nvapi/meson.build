# nvidia only privdes cpp headers
# there is an open pr to provide c support https://github.com/NVIDIA/nvapi/pull/7
project('nvapi', 'cpp',
    version : '550.0.0',
    # all headers and binaries are licensed as all rights reserved by Nvidia
    license : 'LicenseRef-Proprietary',
    license_files : [ 'License.txt' ]
)

fs = import('fs')

cpu = host_machine.cpu_family()
cc = meson.get_compiler('cpp')
root = meson.current_source_dir()

if cpu == 'x86_64'
    libnvapi = cc.find_library('nvapi64', dirs : root / 'amd64')
elif cpu == 'x86'
    libnvapi = cc.find_library('nvapi', dirs : root / 'x86')
else
    error('NVAPI is not supported on ' + cpu)
endif

nvapi_dep = declare_dependency(
    include_directories : '.'
)

# the settings file needs to be compiled as c++
# currently this seems to be the only way to get meson to detect it as c++
# https://github.com/mesonbuild/meson/discussions/11788#discussioncomment-5910024
src = [ fs.copyfile('NvApiDriverSettings.c', 'NvApiDriverSettings.cpp') ]

# this has a couple of magic globals that need to be defined in your own
# headers to get the values from here.
# ```cpp
# extern SettingDWORDNameString mapSettingDWORD[];
# extern SettingWSTRINGNameString mapSettingWSTRING[];
# ```
libnvapi_driver_settings = static_library('NvApiDriverSettings', src,
    dependencies : nvapi_dep,
    build_by_default : false
)

nvapi_driver_settings_dep = declare_dependency(
    link_with : libnvapi_driver_settings,
    include_directories : '.'
)

meson.override_dependency('nvapi', nvapi_dep)
meson.override_dependency('nvapi_driver_settings', nvapi_driver_settings_dep)

install_data(meson.project_license_files(),
    install_dir : get_option('datadir') / 'licenses',
    rename : meson.project_name() + '.LICENSE.txt'
)
