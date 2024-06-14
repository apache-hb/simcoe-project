@echo off
REM Usage: setup.bat builddir buildtype [options]
SET cfgdir="data\meson"

SET builddir=%1
SET buildtype=%2
for /f "tokens=2,* delims= " %%a in ("%*") do set options=%%b
@echo on

meson setup %builddir% --native-file %cfgdir%\base.ini --native-file %cfgdir%\clang-cl.ini --native-file %cfgdir%\%buildtype%.ini %options%
