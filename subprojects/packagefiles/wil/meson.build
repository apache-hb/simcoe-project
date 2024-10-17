project('wil', 'cpp',
    version : '1.0.240803.1',
    # the binary data is licensed under a microsoft private license
    # the code is licensed under the MIT license
    license : 'MIT',
    license_files : [ 'LICENSE' ],
    meson_version : '>=1.1.0',
)

inc = include_directories('include')
wil = declare_dependency(include_directories : inc)
meson.override_dependency('wil', wil)
