# Simcoe

* data\meson\ms-link.ini - use microsoft link.exe
* data\meson\clang-cl.ini - clang-cl and lld-link
* data\meson\shipping.ini - shipping config
* data\meson\trace.ini - compile time tracing

```sh
# debug
meson setup build -Dprefix=%cd%\bundle_debug --native-file data\meson\clang-cl.ini --native-file data\meson\devel.ini

# release
meson setup build-shipping -Dprefix=%cd%\bundle_release--native-file data\meson\clang-cl.ini --native-file data\meson\shipping.ini

# profiling compile time
meson setup build-trace --native-file data\meson\clang-cl.ini --native-file data\meson\trace.ini
python scripts\ninjatracing\ninjatracing.py build\.ninja_log > trace.json

# create package
meson install -C <dir> --skip-subprojects
```


```sh
# building llvm
cmake -S llvm -B build -G Ninja -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra;lld" -DCMAKE_BUILD_TYPE=Release -DLLVM_USE_LINKER="lld-link" -DCMAKE_INSTALL_PREFIX="K:\llvm" -DLLVM_PARALLEL_COMPILE_JOBS=24 -DLLVM_PARALLEL_LINK_JOBS=16 -D CMAKE_C_COMPILER=clang -D CMAKE_CXX_COMPILER=clang++ -DLLVM_HOST_TRIPLE=x86_64
```
