project('fastgltf', 'cpp',
    version : '0.7.2',
    default_options : [
        'cpp_std=c++17'
    ],
    license : 'MIT',
    license_files : [ 'LICENSE.md' ],
    meson_version : '>=1.1.0'
)

simdjson = dependency('simdjson', version : '>=3.3.0')

src = [ 'src/fastgltf.cpp', 'src/base64.cpp' ]
inc = include_directories('include')
args = [ ]

if get_option('custom_small_vector')
    args += [ '-DFASTGLTF_USE_CUSTOM_SMALLVECTOR=1' ]
endif

if get_option('deprecated_exts')
    args += [ '-DFASTGLTF_ENABLE_DEPRECATED_EXT=1' ]
endif

if not get_option('custom_memory_pool')
    args += [ '-DFASTGLTF_DISABLE_CUSTOM_MEMORY_POOL=1' ]
endif

if get_option('double_precision')
    args += [ '-DFASTGLTF_USE_64BIT_FLOAT=1' ]
endif

libgltf = library('fastgltf', src,
    install : not meson.is_subproject(),
    dependencies : simdjson,
    cpp_args : args + [ '-D_SILENCE_CXX20_U8PATH_DEPRECATION_WARNING' ],
    include_directories : inc
)

fastgltf_dep = declare_dependency(
    include_directories : inc,
    link_with : libgltf,
    compile_args : args
)

meson.override_dependency('fastgltf', fastgltf_dep)

headers = [
    'include/fastgltf/core.hpp',
    'include/fastgltf/util.hpp',
    'include/fastgltf/types.hpp',
    'include/fastgltf/tools.hpp',
    'include/fastgltf/base64.hpp',
    'include/fastgltf/glm_element_traits.hpp'
]

install_headers(headers, subdir : 'fastgltf')
