i18n_include = include_directories('.', 'include')

i18n_dependencies = [ core ]

src = [
    'src/i18n.cpp',
    reflect.process('data/i18n.ct')
]

libi18n = library('i18n', src,
    cpp_args : [ '-DSM_I18N_BUILD=1' ],
    include_directories : [ i18n_include, reflect_include ],
    dependencies : i18n_dependencies
)

i18n = declare_dependency(
    link_with : libi18n,
    include_directories : [
        i18n_include, reflect_include, libi18n.private_dir_include()
    ],
    dependencies : i18n_dependencies
)
