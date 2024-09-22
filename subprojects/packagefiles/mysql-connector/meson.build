project('mysql-connector', 'cpp',
    version : '9.0.0',
    license : 'GPL-2.0',
    license_files : [ 'LICENSE.txt' ],
    meson_version : '>=1.1.0',
)

add_project_arguments([ '-D_CRT_SECURE_NO_WARNINGS', '-D_SCL_SECURE_NO_WARNINGS' ], language : 'cpp')

src = [
    'common/collection.cc',
    'common/result.cc',
    'common/session.cc',
    'common/value.cc',

    'devapi/crud.cc',
    'devapi/document.cc',
    'devapi/result.cc',
    'devapi/session.cc',

    'cdk/'
]

openssl = dependency('openssl')

inc = include_directories('include', 'cdk/include')

libdevapi = static_library('devapi', src,
    dependencies : openssl,
    include_directories : inc
)

devapi = declare_dependency(
    link_with : libdevapi,
    include_directories : inc
)

meson.override_dependency('devapi', devapi)
