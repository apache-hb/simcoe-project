# Simcoe

## How to build

1. ensure you have the absolute latest version of vs2022
    * you need the *latest*, only the most recent release will work

2. build this fork of llvm https://github.com/apache-hb/llvm-project/tree/extensions
    * build the extension branch, and the llvm;clang;lld targets
    * ensure the target bin directory is on the %PATH%

3. install dependencies
    * winflexbison3
    * imagemagick
    * meson 1.5.2
    * ninja build
    * python3.12
    * openssl

4. configure build tree with `setup.bat build devel`

5. build using `ninja -C build`
    * test with `ninja -C build test`
    * benchmark with `ninja -C build benchmark`
    * `.\build\editor.exe --bundle .\build\bundle.tar`
