project('microsoft-gdk',
    # the gdk doesnt use semantic versioning
    # this is just what nuget says in the version field
    version : '10.25398.1897',
    license : 'LicenseRef-Proprietary',
    license_files : [ 'LICENSE.md' ],
    meson_version : '>=1.1.0'
)

host_cpu = host_machine.cpu_family()
cpp = meson.get_compiler('cpp')
progdir = 'native/bin'
kitdir = 'native/231000/GRDK/gamekit'

if host_cpu != 'x86_64'
    error('gdk only distributes amd64 binaries. ' + host_cpu + ' is not supported.')
endif

bindir = kitdir / 'lib/amd64'
incdir = kitdir / 'Include'

shared_include = include_directories(incdir)

libgameruntime = cpp.find_library('xgameruntime', dirs : bindir)
libgameinput = cpp.find_library('GameInput', dirs : bindir)

gameruntime = declare_dependency(
    dependencies : [ libgameruntime ],
    include_directories : shared_include
)

gameinput = declare_dependency(
    dependencies : [ libgameinput ],
    include_directories : shared_include
)

wdapp = find_program('wdapp', dirs : progdir)
makepkg = find_program('makepkg', dirs : progdir)
makepri = find_program('makepri', dirs : progdir)
packageutil = find_program('packageutil', dirs : progdir)
splitpkg = find_program('splitpkg', dirs : progdir)

meson.override_dependency('gameruntime', gameruntime)
meson.override_dependency('gameinput', gameinput)

meson.override_find_program('wdapp', wdapp)
meson.override_find_program('makepkg', makepkg)
meson.override_find_program('makepri', makepri)
meson.override_find_program('packageutil', packageutil)
meson.override_find_program('splitpkg', splitpkg)
