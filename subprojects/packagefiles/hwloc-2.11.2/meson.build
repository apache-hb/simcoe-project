project('hwloc', 'c',
    version : '2.11.2',
    meson_version : '>=1.3.0',
    license : 'BSD-3-Clause',
    license_files : 'COPYING',
)

hwloc_include = include_directories('include')
os = host_machine.system()
arch = host_machine.cpu_family()
cc = meson.get_compiler('c')
fs = import('fs')
cc = meson.get_compiler('c')

deps = [ ]

src = [
    'hwloc/base64.c',
    'hwloc/bind.c',
    'hwloc/bitmap.c',
    'hwloc/components.c',
    'hwloc/cpukinds.c',
    'hwloc/diff.c',
    'hwloc/distances.c',
    'hwloc/memattrs.c',
    'hwloc/misc.c',
    'hwloc/pci-common.c',
    'hwloc/shmem.c',
    'hwloc/topology-noos.c',
    'hwloc/topology-synthetic.c',
    'hwloc/topology-xml-nolibxml.c',
    'hwloc/topology-xml.c',
    'hwloc/topology.c',
    'hwloc/traversal.c'
]

libxml2 = dependency('libxml-2.0', required : get_option('libxml2'))

if libxml2.found()
    src += [ 'hwloc/topology-xml-libxml.c' ]
    deps += [ libxml2 ]
endif

if os == 'windows'
    src += [ 'hwloc/topology-windows.c' ]
elif os == 'linux'
    src += [ 'hwloc/topology-linux.c', 'hwloc/topology-hardwired.c' ]
endif

if arch.contains('x86')
    src += [ 'hwloc/topology-x86.c' ]
endif

version_parts = meson.project_version().split('.')

if os == 'linux'
    cdata = configuration_data()
    cdata.set_quoted('HWLOC_VERSION', meson.project_version())
    cdata.set('HWLOC_VERSION_MAJOR', version_parts[0])
    cdata.set('HWLOC_VERSION_MINOR', version_parts[1])
    cdata.set('HWLOC_VERSION_RELEASE', version_parts[2])
    cdata.set_quoted('HWLOC_VERSION_GREEK', '')
    cdata.set('HWLOC_HAVE_WINDOWS_H', os == 'windows')
    cdata.set('HWLOC_DECLSPEC', '')

    if os == 'windows'
        cdata.set('hwloc_pid_t', 'HANDLE')
        cdata.set('hwloc_thread_t', 'HANDLE')
    elif os == 'linux'
        cdata.set('hwloc_pid_t', 'pid_t')
        cdata.set('hwloc_thread_t', 'pthread_t')
    endif

    cdata.set10('HWLOC_HAVE_STDINT_H', true)
    cdata.set('HWLOC_SYM_PREFIX', 'hwloc_')
    cdata.set('HWLOC_SYM_PREFIX_CAPS', 'HWLOC_')

    cdata.set('HWLOC_SIZEOF_UNSIGNED_LONG', cc.sizeof('unsigned long'))
    cdata.set('HWLOC_SIZEOF_UNSIGNED_INT', cc.sizeof('unsigned int'))
    cdata.set('SIZEOF_VOID_P', cc.sizeof('void *'))

    hwloc_autogen_config = configure_file(
        output : 'hwloc_autogen_config.h',
        configuration : cdata
    )
elif os == 'windows'
    hwloc_autogen_config = fs.copyfile('contrib/windows/hwloc_config.h', 'hwloc_autogen_config.h')
endif

if os == 'linux'
    defines = [
        '_SC_NPROCESSORS_CONF',
        '_SC_NPROC_CONF',
        '_SC_NPROCESSORS_ONLN',
        '_SC_NPROC_ONLN',
        '_SC_NPROCESSORS_CONF',
        '_SC_NPROC_CONF',
        '_SC_PAGE_SIZE',
        '_SC_PAGESIZE'
    ]

    headers = [
        'stdint.h',
        'ctype.h',
        'dirent.h',
        'malloc.h',
        'stdint.h',
        'stdio.h',
        'stdlib.h',
        'strings.h',
        'string.h',
        'sys/mman.h',
        'sys/stat.h',
        'sys/types.h',
        'sys/utsname.h',
        'sys/sysctl.h',
        'sys/param.h',
        'unistd.h',
        'malloc.h',
    ]

    cdata = configuration_data()
    cdata.set10('HAVE_SSIZE_T', true)
    cdata.set10('HAVE_STRCASECMP', true)
    cdata.set10('HWLOC_HAVE_CPU_SET', true)
    cdata.set10('HWLOC_HAVE_CPU_SET_S', true)
    cdata.set10('HWLOC_LINUX_SYS', true)
    cdata.set10('HWLOC_HAVE_LINUXIO', true)
    cdata.set10('HWLOC_X86_64_ARCH', arch == 'x86_64')

    foreach header : headers
        cdata.set('HAVE_' + header.replace('/', '_').replace('.', '_').to_upper(), cc.has_header(header))
    endforeach

    foreach define : defines
        cdata.set10('HAVE_DECL_' + define.to_upper(), cc.has_define(define, prefix : '#include <unistd.h>'))
    endforeach

    cdata.set10('HAVE_OPENAT', cc.has_function('openat', prefix : '#include <fcntl.h>'))

    cdata.set10('HAVE_DECL_FABSF', true)
    cdata.set10('HAVE_DECL_MODFF', true)
    cdata.set10('HWLOC_HAVE_PTHREAD_MUTEX', true)

    cdata.set10('HWLOC_HAVE_SYSCALL', true)
    cdata.set10('HWLOC_HAVE_SCHED_SETAFFINITY', true)

    hwloc_private_config = configure_file(
        output : 'hwloc_private_config.h',
        configuration : cdata
    )
elif os == 'windows'
    hwloc_private_config = fs.copyfile('contrib/windows/private_config.h', 'hwloc_private_config.h')
endif

args = cc.get_supported_arguments(
    '-Wno-unused-parameter',
    '-Wno-macro-redefined',
    '-Wno-cast-function-type-mismatch',
    '-Wno-deprecated-declarations'
)

libhwloc = library('hwloc', src, hwloc_autogen_config, hwloc_private_config,
    c_args : args + [ '-D_GNU_SOURCE', '-DRUNSTATEDIR="/var/run"', '-D_CRT_SECURE_NO_WARNINGS' ],
    include_directories : [ hwloc_include ],
    dependencies : deps,
    override_options : [ 'unity=off' ],
)

hwloc = declare_dependency(
    link_with : libhwloc,
    include_directories : [ hwloc_include ],
    dependencies : [ deps ]
)

meson.override_dependency('hwloc', hwloc)
