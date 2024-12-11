project('fuzztest', 'cpp',
    default_options : [
        'cpp_std=c++17',
    ],
    version : '2024.10.28',
    license : 'BSD3',
    license_files : 'LICENSE',
    meson_version : '>=1.1.0',
)

cpp = meson.get_compiler('cpp')
override_options = [ 'unity=off' ]

src = [
    'fuzztest/internal/any_test.cc',
    'fuzztest/internal/compatibility_mode.cc',
    'fuzztest/internal/configuration.cc',
    'fuzztest/internal/corpus_database.cc',
    'fuzztest/internal/io.cc',
    'fuzztest/internal/coverage.cc',
    'fuzztest/internal/fixture_driver.cc',
    'fuzztest/internal/googletest_adaptor.cc',
    'fuzztest/internal/logging.cc',
    'fuzztest/internal/registry.cc',
    'fuzztest/internal/runtime.cc',
    'fuzztest/internal/seed_seq.cc',
    'fuzztest/internal/serialization.cc',
    'fuzztest/internal/status.cc',
    'fuzztest/internal/subprocess.cc',
    'fuzztest/internal/type_support.cc',
]

deps = [
    dependency('gtest'),
    dependency('gmock'),
    dependency('absl_status'),
    dependency('absl_random'),
    dependency('absl_log'),
    dependency('absl_container'),
    dependency('absl_algorithm_container'),
    dependency('absl_strings'),
    dependency('re2')
]

cpp_args = cpp.get_supported_arguments([
    '-Wno-sign-compare',
    '-Wno-unused-parameter',
    '-Wno-missing-field-initializers',
    '-Wno-deprecated-declarations'
])

libfuzztest = library('fuzztest', src,
    cpp_args : cpp_args,
    include_directories : '.',
    dependencies : deps,
    override_options: override_options,
)

fuzztest_dep = declare_dependency(
    link_with : libfuzztest,
    include_directories : '.',
    dependencies : deps
)

meson.override_dependency('fuzztest', fuzztest_dep)
