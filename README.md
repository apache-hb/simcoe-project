# Simcoe

* data\meson\ms-link.ini - use microsoft link.exe
* data\meson\clang-cl.ini - clang-cl and lld-link
* data\meson\shipping.ini - shipping config
* data\meson\trace.ini - compile time tracing

```sh
# debug
meson setup build -Dprefix=%cd%\bundle_debug --native-file data\meson\clang-cl.ini

# release
meson setup build-release -Dbuildtype=release -Dprefix=%cd%\bundle_release -Db_lto=true --native-file data\meson\clang-cl.ini

# profiling compile time
meson setup build-trace --native-file data\meson\clang-cl.ini --native-file data\meson\trace.ini
python scripts\ninjatracing\ninjatracing.py build\.ninja_log > trace.json

# create package
meson install -C <dir> --skip-subprojects
```
