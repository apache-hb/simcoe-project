# Windows gamedev and meson

## Redists and binary only dependencies?

### Redistributables

A large amount of microsoft sdks are distributed as a binary only .lib + .dll pair
along with header files. Common patterns in subproject meson files, possible improvements?

```py
# subproject/d3d12-agility/meson.build
redist_dll = files(bindir / 'D3D12Core.dll', bindir / 'D3D12SDKLayers.dll')
redist_pdb = files(bindir / 'D3D12Core.pdb', bindir / 'd3d12SDKLayers.pdb')

# meson.build
agility = subproject('d3d12-agility')
agility_redist = agility.get_variable('redist_dll')
install_data(agility_redist,
    install_tag : 'runtime',
    install_dir : get_option('bindir') / 'D3D12'
)

if get_option('buildtype') != 'release'
    agility_redist_pdb = agility.get_variable('redist_pdb')
    install_data(agility_redist_pdb,
        install_tag : 'devel',
        install_dir : get_option('bindir') / 'D3D12'
    )
endif

executable('game', 'main.cpp',
    install : true
)
```

Its very common for pdbs or dlls to be development only, and
disallowed from redistribution. Repeated `install_data` and `get_variable('redist_(dll|pdb)')`
start adding up when using alot of binary only dependencies.
Some possible solutions...

```py
# subproject/d3d12-agility/meson.build
redist_dll = files(bindir / 'D3D12Core.dll', bindir / 'D3D12SDKLayers.dll')
redist_pdb = files(bindir / 'D3D12Core.pdb', bindir / 'd3d12SDKLayers.pdb')

redist = [ redist_dll ]

if get_option('buildtype') != 'release'
    redist += [ redist_pdb ]
endif

agility = declare_dependency(redist_files : redist)

meson.override_dependency('agility', agility)

# d3d12-agility.wrap
[provides]
dependency_names = agility

# meson.build
agility = dependency('agility')

executable('game', 'main.cpp',
    dependencies : [ agility ],
    install : true
)
```

Pros:
* Shifts responsibility of managing redist installation to the subproject
* Integrates with existing dependency/wrap system

Cons:
* Some redists require that they are in specific directories relative to the binary.
    * maybe a `redist:` parameter when declaring build targets that accepts a dict
      representing the directory heirarchy needed during installs

### Binary only libraries with no language requirements

Most microsoft apis are provided in a single library with headers that worth with C or C++.
A problem arises when writing a subproject without knowledge of the language the parent project
is using.

```py
project('only-lib-files', # no language! we dont compile any source
    version : '1.2.3'
)

# we need to make dependencies via `compiler.find_library`

# what if the parent project doesnt use C?
cc = meson.get_compiler('c')

# what if the parent project doesnt use C++?
cpp = meson.get_compiler('cpp')

# what if the parent project uses neither?
# some languages support generating api definitions from
# C headers. zig has @cImport. rust can generate defs
# via c2rust and consume the lib files via those.
```

Some ideas...

```py
# find the library directly
# perhaps have options for attaching the header files
# for this library to provide its consumers.
lib = meson.find_library('the_library')
```

Pros: known vocabulary of `find_library`
Cons: specifying (or validating) linkage of the object would be hard

```py
# get a compiler that can handle `find_library`
cc = meson.object_compiler()
```

pros:
    works with existing `compiler.find_library` code
    could be used in existing subprojects that are binary only

cons:
    non obvious behaviour if no compiler that can handle precompiled objects is being used.

## install overriding build_by_default for the default build target
||Sorry for bringing this up again...||

Currently marking a target as `install` means it is always built during a default build.
This is suboptimal during incremental development as it may mean very large libraries
that are only required at release are built. More importantly there is no way to not build
`install` libraries.

A possible solution, that preserves backwards compatibility, would be to
provide a `dev/devel` (name pending) target that only builds libraries that
are marked as `build_by_default : true`. This would not break the existing
behaviour of `all` building everything required for an install so `install` does
not build with anything administration rights.

Pros: backwards compatible, reduces incremental build times
Cons: an extra, possibly quite large, ninja rule

## Support for build time profiling

Large projects large build times.
With clangs -ftime-trace and the generated .ninja_log file its possible to diagnose
Slow to build files, or dependencies that can be broken to improve parallelism.
Currently its possible to use the time-trace flags directly but requires different setup
for both msvc and clang.
msvc uses [vcperf](https://github.com/microsoft/vcperf) to do its tracing
wheras clang uses `-ftime-trace` (and optionally [ninjatracing](https://github.com/nico/ninjatracing) to collate the results)
gcc has `-ftime-report` but thats more aimed at profiling the compiler itself.

A possible solution would be adding a `b_time_build` flag or simmilar to meson
that would enable the relevant compiler flags for clang, as well as maybe adding targets
for interacting with vcperf or ninjatracing if they're available.

## Beating badly behaved dependencies into submission

When doing alot of full rebuilds which is common when deploying to a wide range of targets.
Ending up with possibly `arch * platform * storefront * ...` full builds required to make a full
suite of archives. Small parts of dependencies can add up to noticable build slowdown,
Some dependencies always build their tests, example programs,
libraries you dont need that arent `build_by_default : false`,
or search for programs to build optional features like doxygen, or generating pkg-config files.

Some possible (bandaid) fixes:

* Allow the root project to override the `build_by_default` and `install` values for an entire subproject, or per subproject target
* Update `meson.override_find_program` and `meson.override_dependency` to accept disabler objects to mark the dep/program as not wanted.
* Add a set of "root targets" in the main project and let ninja shake out all the targets
  that are not absolutely necessary.
    * This is possible by specifying the exact target names, but this is fragile
      when you have a large or frequently changing set of targets.
* allow completley disabling pkg-config/cmake searching
    * on windows just searching for cmake can take up to 2 seconds (!!!) depending on how its configured.

None of these solutions feel particularly great but would solve some of the configure and build time issues

## Installing the outputs of generators

As theres no way to add a new language wholesale to meson i've found
the best way of handling shader objects is providing a `generator` to create them.
Writing a `custom_target` per shader would become unweildy especially with [how many you might have](https://therealmjp.github.io/posts/shader-permutations-part1/).
Begin able to install the result of `generator.process` via`install_data` would solve this quite handily.
