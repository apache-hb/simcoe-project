# #############################################################################
# Copyright (c) 2018-present     Dima Krasner <dima@dimakrasner.com>
#                                lzutao <taolzu(at)gmail.com>
# All rights reserved.
#
# This source code is licensed under both the BSD-style license (found in the
# LICENSE file in the root directory of this source tree) and the GPLv2 (found
# in the COPYING file in the root directory of this source tree).
# #############################################################################

project('zstd',
  ['c', 'cpp'],
  license: ['BSD', 'GPLv2'],
  default_options : [
    # There shouldn't be any need to force a C standard convention for zstd
    # but in case one would want that anyway, this can be done here.
    # 'c_std=gnu99',
    # c++11 standard is useful for pzstd
    'cpp_std=c++11',
    'buildtype=release',
    'warning_level=3',
    'unity=off',
    'unity_size=1',
    # -Wdocumentation does not actually pass, nor do the test binaries,
    # so this isn't safe
    #'werror=true'
  ],
  version: '1.5.6',
  meson_version: '>=1.1.0',
  license : 'BSD',
  license_files : 'LICENSE'
)

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
pkgconfig = import('pkgconfig')
windows_mod = import('windows')

host_machine_os = host_machine.system()
os_windows = 'windows'
os_linux = 'linux'
os_darwin = 'darwin'
os_freebsd = 'freebsd'
os_sun = 'sunos'

cc_id = cc.get_id()
compiler_gcc = 'gcc'
compiler_clang = 'clang'
compiler_msvc = 'msvc'

zstd_version = meson.project_version()

zstd_libversion = zstd_version

# =============================================================================
# Installation directories
# =============================================================================

zstd_prefix = get_option('prefix')
zstd_bindir = get_option('bindir')
zstd_datadir = get_option('datadir')
zstd_mandir = get_option('mandir')
zstd_docdir = join_paths(zstd_datadir, 'doc', meson.project_name())

# =============================================================================
# Project options
# =============================================================================

# Built-in options
use_debug = get_option('debug')
buildtype = get_option('buildtype')
default_library_type = get_option('default_library')

# Custom options
debug_level = get_option('debug_level')
legacy_level = get_option('legacy_level')
use_backtrace = get_option('backtrace')
use_static_runtime = get_option('static_runtime')

bin_programs = get_option('bin_programs')
bin_contrib = get_option('bin_contrib')
bin_tests = get_option('bin_tests')

feature_multi_thread = get_option('multi_thread')
feature_zlib = get_option('zlib')
feature_lzma = get_option('lzma')
feature_lz4 = get_option('lz4')

# =============================================================================
# Dependencies
# =============================================================================

libm_dep = cc.find_library('m', required: false)
if host_machine_os == os_windows
  thread_dep = dependency('', required: false)
  use_multi_thread = not feature_multi_thread.disabled()
else
  thread_dep = dependency('threads', required: feature_multi_thread)
  use_multi_thread = thread_dep.found()
endif
# Arguments in dependency should be equivalent to those passed to pkg-config
zlib_dep = dependency('zlib', required: feature_zlib)
use_zlib = zlib_dep.found()
lzma_dep = dependency('liblzma', required: feature_lzma)
use_lzma = lzma_dep.found()
lz4_dep = dependency('liblz4', required: feature_lz4)
use_lz4 = lz4_dep.found()

# =============================================================================
# Compiler flags
# =============================================================================

add_project_arguments('-DXXH_NAMESPACE=ZSTD_', language: ['c'])

pzstd_warning_flags = []
if [compiler_gcc, compiler_clang].contains(cc_id)
  common_warning_flags = [ '-Wundef', '-Wshadow', '-Wcast-align', '-Wcast-qual' ]
  pzstd_warning_flags = ['-Wno-shadow', '-Wno-deprecated-declarations']
  if cc_id == compiler_clang
    common_warning_flags += ['-Wconversion', '-Wno-sign-conversion', '-Wdocumentation']
  endif
  cc_compile_flags = cc.get_supported_arguments(common_warning_flags + ['-Wstrict-prototypes'])
  cxx_compile_flags = cxx.get_supported_arguments(common_warning_flags)
  add_project_arguments(cc_compile_flags, language : 'c')
  add_project_arguments(cxx_compile_flags, language : 'cpp')
elif cc_id == compiler_msvc
  msvc_compile_flags = [ '/D_UNICODE', '/DUNICODE' ]
  if use_multi_thread
    msvc_compile_flags += '/MP'
  endif
  if use_static_runtime
    msvc_compile_flags += '/MT'
  endif
  add_project_arguments(msvc_compile_flags, language: ['c', 'cpp'])
endif

# =============================================================================
# Subdirs
# =============================================================================

subdir('build/meson/lib')

if bin_programs or bin_tests
  subdir('build/meson/programs')
endif

if bin_tests
  subdir('build/meson/tests')
endif

if bin_contrib
  subdir('build/meson/contrib')
endif

install_data(meson.project_license_files(),
    install_dir : get_option('licensedir'),
    rename : meson.project_name() + '.LICENSE.txt',
    install_tag : 'legal',
)
