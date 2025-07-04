# Copyright (c) 2022-2023, PostgreSQL Global Development Group

# test/ is entered via top-level meson.build, that way it can use the default
# args for executables (which depend on libpq).

libpq_sources = files(
  'fe-auth-scram.c',
  'fe-auth.c',
  'fe-connect.c',
  'fe-exec.c',
  'fe-lobj.c',
  'fe-misc.c',
  'fe-print.c',
  'fe-protocol3.c',
  'fe-secure.c',
  'fe-trace.c',
  'legacy-pqsignal.c',
  'libpq-events.c',
  'pqexpbuffer.c',
)
libpq_so_sources = [] # for shared lib, in addition to the above

if host_system == 'windows'
  libpq_sources += files('pthread-win32.c', 'win32.c')
  libpq_so_sources += rc_lib_gen.process(win32ver_rc, extra_args: [
    '--NAME', 'libpq',
    '--FILEDESC', 'PostgreSQL Access Library',])
endif

if ssl.found()
  libpq_sources += files('fe-secure-common.c')
  libpq_sources += files('fe-secure-openssl.c')
endif

if gssapi.found()
  libpq_sources += files(
    'fe-gssapi-common.c',
    'fe-secure-gssapi.c',
  )
endif

export_file = custom_target('libpq.exports',
  kwargs: gen_export_kwargs,
)

# port needs to be in include path due to pthread-win32.h
libpq_inc = include_directories('.', '../../port')
libpq_c_args = ['-DSO_MAJOR_VERSION=5', '-Wno-tautological-pointer-compare']

# Not using both_libraries() here as
# 1) resource files should only be in the shared library
# 2) we want the .pc file to include a dependency to {pgport,common}_static for
#    libpq_st, and {pgport,common}_shlib for libpq_sh
#
# We could try to avoid building the source files twice, but it probably adds
# more complexity than its worth (AIX doesn't support link_whole yet, reusing
# object files requires also linking to the library on windows or breaks
# precompiled headers).
libpq_st = static_library('libpq',
  libpq_sources,
  include_directories: [libpq_inc],
  c_args: libpq_c_args,
  c_pch: pch_postgres_fe_h,
  dependencies: [frontend_stlib_code, libpq_deps, common_deps],
  kwargs: default_lib_args,
)

libpq_so = shared_library('libpq',
  libpq_sources + libpq_so_sources,
  include_directories: [libpq_inc, postgres_inc],
  c_args: libpq_c_args,
  c_pch: pch_postgres_fe_h,
  version: '5.' + pg_version_major.to_string(),
  soversion: host_system != 'windows' ? '5' : '',
  darwin_versions: ['5', '5.' + pg_version_major.to_string()],
  dependencies: [frontend_shlib_code, libpq_deps, common_deps],
  link_depends: export_file,
  link_args: export_fmt.format(export_file.full_path()),
  kwargs: default_lib_args,
)

libpq = declare_dependency(
  link_with: [libpq_st],
  include_directories: [include_directories('.', '../../include')]
)

meson.override_dependency('libpq', libpq)

# pkgconfig.generate(
#   name: 'libpq',
#   description: 'PostgreSQL libpq library',
#   url: pg_url,
#   libraries: libpq,
#   libraries_private: [frontend_stlib_code, libpq_deps],
# )

install_headers(
  'libpq-fe.h',
  'libpq-events.h',
)

install_headers(
  'libpq-int.h',
  'pqexpbuffer.h',
  'fe-auth-sasl.h',
  install_dir: dir_include_internal,
)
install_data('pg_service.conf.sample',
  install_dir: dir_data,
)

tests += {
  'name': 'libpq',
  'sd': meson.current_source_dir(),
  'bd': meson.current_build_dir(),
  'tap': {
    'tests': [
      't/001_uri.pl',
      't/002_api.pl',
      't/003_load_balance_host_list.pl',
      't/004_load_balance_dns.pl',
    ],
    'env': {'with_ssl': ssl_library},
  },
}

subdir('po', if_found: libintl)
