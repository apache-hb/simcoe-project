# Simcoe

```sh
# debug
meson setup build -Dprefix=%cd%\bundle_debug --native-file data\meson\clang-cl.ini

# release
meson setup build-release -Dbuildtype=release -Dprefix=%cd%\bundle_release -Db_lto=true --native-file data\meson\clang-cl.ini

# profiling compile time
meson setup build --native-file data\meson\clang-cl.ini --native-file data\meson\trace.ini

python scripts\ninjatracing\ninjatracing.py build\.ninja_log > trace.json
```
