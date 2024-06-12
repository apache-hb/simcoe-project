
## Ignore stuff below here, not important

* data\meson\ms-link.ini - use microsoft link.exe
* data\meson\clang-cl.ini - clang-cl and lld-link
* data\meson\shipping.ini - shipping config
* data\meson\trace.ini - compile time tracing

```sh
# debug
meson setup build -Dprefix=%cd%\bundle_debug --native-file data\meson\clang-cl.ini --native-file data\meson\devel.ini

# private release
meson setup build-private -Dprefix=%cd%\bundle_private --native-file data\meson\clang-cl.ini --native-file data\meson\private.ini

# release
meson setup build-release -Dprefix=%cd%\bundle_release --native-file data\meson\clang-cl.ini --native-file data\meson\shipping.ini

# profiling compile time
meson setup build-trace --native-file data\meson\clang-cl.ini --native-file data\meson\trace.ini
python scripts\ninjatracing\ninjatracing.py build\.ninja_log > trace.json

# create package
meson install -C <dir> --skip-subprojects
```


```sh
# blender
cmake -S . -B build -G Ninja -DCMAKE_LINKER="lld-link" -DCMAKE_C_COMPILER="clang-cl" -DCMAKE_CXX_COMPILER="clang-cl" -DCMAKE_INSTALL_PREFIX="K:\blender" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

```sh
cmake -S llvm -B build-debug -G Ninja -DLLVM_ENABLE_PROJECTS="llvm;clang;clang-tools-extra" -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX="K:\llvm-debug" -DLLVM_TARGETS_TO_BUILD=X86 "-DCMAKE_TOOLCHAIN_FILE=K:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_LINKER="lld-link" -DCMAKE_C_COMPILER="clang-cl" -DCMAKE_CXX_COMPILER="clang-cl" -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_RC_COMPILER="llvm-rc.exe"

# build llvm for local use
cmake -S llvm -B build -G Ninja -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld;lldb" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="K:\llvm" -DLLVM_TARGETS_TO_BUILD="X86" -DCMAKE_TOOLCHAIN_FILE="K:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_LINKER="lld-link" -DCMAKE_C_COMPILER="clang-cl" -DCMAKE_CXX_COMPILER="clang-cl" -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded" -DLLVM_INCLUDE_TOOLS=ON -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="DirectX" -DCMAKE_CXX_FLAGS="-march=native -mtune=native" -DCMAKE_C_FLAGS="-march=native -mtune=native" -DCLANG_ENABLE_HLSL=ON -DCMAKE_RC_COMPILER="llvm-rc.exe" -DLLVM_ENABLE_LTO=Thin

# build llvm for local use + time trace
cmake -S llvm -B build -G Ninja -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld;openmp;compiler-rt" -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX="K:\llvm"  -DLLVM_TARGETS_TO_BUILD="X86" "-DCMAKE_TOOLCHAIN_FILE=K:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DCMAKE_LINKER="lld-link" -DCMAKE_C_COMPILER="clang-cl" -DCMAKE_CXX_COMPILER="clang-cl" -DLLVM_ENABLE_LTO="Thin" -DLLVM_INTEGRATED_CRT_ALLOC="K:\github\rpmalloc" -DCMAKE_MSVC_RUNTIME_LIBRARY="MultiThreaded" -DLLVM_INCLUDE_TOOLS=ON -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="DirectX" -DCMAKE_CXX_FLAGS="-march=native -mtune=native -ftime-trace" -DCMAKE_C_FLAGS="-march=native -mtune=native -ftime-trace" -DCLANG_ENABLE_HLSL=ON -DCMAKE_CL_SHOWINCLUDES_PREFIX="Note: including file: " -DCMAKE_RC_COMPILER="llvm-rc.exe"

# building llvm
cmake -S llvm -B build-stage0 -G Ninja -DLLVM_ENABLE_PROJECTS="clang;lld" -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX="K:\llvm" -DLLVM_PARALLEL_LINK_JOBS=16 -DLLVM_HOST_TRIPLE=x86_64 -DLLVM_TARGETS_TO_BUILD=X86

# stage 1
cmake -S llvm -B build -G Ninja -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="K:\llvm" -DLLVM_PARALLEL_LINK_JOBS=16 -DLLVM_HOST_TRIPLE=x86_64 -DLLVM_TARGETS_TO_BUILD=X86 -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DLLVM_ENABLE_LIBCXX=YES -DLLVM_ENABLE_LTO=Thin

cmake -G Ninja -S runtimes -B build-libcxx -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DLLVM_ENABLE_RUNTIMES=libcxx -DCMAKE_INSTALL_PREFIX="K:\llvm" -DCMAKE_BUILD_TYPE=Release -DLIBCXX_ENABLE_SHARED=YES -DLIBCXX_ENABLE_STATIC=NO -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=NO -DLIBCXX_ENABLE_EXCEPTIONS=NO -DLIBCXX_ENABLE_RTTI=NO -DLIBCXX_INCLUDE_TESTS=NO -DLIBCXX_INCLUDE_BENCHMARKS=NO -DLIBCXXABI_USE_LLVM_UNWINDER=NO -DLLVM_DIR="K:\llvm\lib\cmake\llvm"

# stage 1 libcxx static
cmake -S runtimes -B build-libcxx-static -G Ninja -DLLVM_ENABLE_RUNTIMES="libcxx" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="K:\llvm-static" -DLLVM_PARALLEL_COMPILE_JOBS=24 -DLLVM_PARALLEL_LINK_JOBS=16 -DLIBCXX_ENABLE_SHARED=NO -DLIBCXX_ENABLE_STATIC=YES -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=NO -DLIBCXX_ENABLE_EXCEPTIONS=NO -DLIBCXX_ENABLE_RTTI=NO -DLIBCXX_INCLUDE_TESTS=NO -DLIBCXX_INCLUDE_BENCHMARKS=NO -DLIBCXXABI_USE_LLVM_UNWINDER=NO -DLIBCXX_NO_VCRUNTIME=YES -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DLLVM_DIR="K:\llvm\lib\cmake\llvm" -DLIBCXX_HERMETIC_STATIC_LIBRARY=YES

# stage 1 libcxx shared
cmake -S runtimes -B build-libcxx-shared -G Ninja -DLLVM_ENABLE_RUNTIMES="libcxx" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="K:\llvm-shared" -DLLVM_PARALLEL_COMPILE_JOBS=24 -DLLVM_PARALLEL_LINK_JOBS=16 -DLIBCXX_ENABLE_SHARED=YES -DLIBCXX_ENABLE_STATIC=NO -DLIBCXX_ENABLE_TIME_ZONE_DATABASE=NO -DLIBCXX_ENABLE_EXCEPTIONS=NO -DLIBCXX_ENABLE_RTTI=NO -DLIBCXX_INCLUDE_TESTS=NO -DLIBCXX_INCLUDE_BENCHMARKS=NO -DLIBCXXABI_USE_LLVM_UNWINDER=NO -DLIBCXX_NO_VCRUNTIME=YES -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl -DLLVM_DIR="K:\llvm\lib\cmake\llvm"
```
