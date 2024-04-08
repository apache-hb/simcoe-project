$llvm_url = "https://elliot-static-site.s3.amazonaws.com/data/llvm.zip"
$meson_url = "https://elliot-static-site.s3.amazonaws.com/data/meson.zip"

$cwd = Get-Location

# cachedir is out root directory, everything we do happens in here
$cachedir = "$cwd/.buildcache"

# packagecache is the directory where we store downloaded zip files
$packagecache = "$cachedir/packages"

# datacache is the directory where we store extracted files
$datacache = "$cachedir/data"

# Add VS Devenv
Import-Module "$env:ProgramFiles\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "$env:ProgramFiles\Microsoft Visual Studio\2022\Community" -DevCmdArguments '-arch=x64'

# set the current directory to the old one
Set-Location $cwd

# Ensure a directory exists, creating it if it doesn't
function Ensure-Directory {
    param(
        [string]$dir
    )

    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Path $dir
    }
}

# Download a file and extract it to the cache directory
# only if it doesn't already exist
function Cache-File {
    param(
        [string]$url,
        [string]$filename
    )

    # packagedir is the directory where the file will be extracted
    $packagedir = "$datacache/$filename"

    # zipfile is the path to the downloaded zip file
    $zipfile = "$packagecache/$filename.zip"
    if (-not (Test-Path $zipfile)) {
        # download the file
        Invoke-WebRequest -Uri $url -OutFile $zipfile

        # extract the file
        Expand-Archive -Path $zipfile -Destination $packagedir
    }
}

# ensure all the directories we need exist
Ensure-Directory $cachedir
Ensure-Directory $packagecache
Ensure-Directory $datacache

# Cache the LLVM and Meson packages
Cache-File -url $llvm_url -filename "llvm"
Cache-File -url $meson_url -filename "meson"

$llvmdir = "$datacache/llvm/llvm"
$bindir = "$llvmdir/bin"

# install python using winget
& "winget" install -e -i --id=9NCVDN91XZQP --source=msstore

# install ninja using pip
& "python" -m pip install ninja

# create a native file for meson that references the cached LLVM
$cfgpath = "$cachedir/meson-native.ini"
$config = @"
[constants]
llvmdir = '$llvmdir'
llvmbindir = '$bindir'

[binaries]
c = llvmbindir + '\\clang-cl.exe'
cpp = llvmbindir + '\\clang-cl.exe'
ar = llvmbindir + '\\llvm-ar.exe'
strip = llvmbindir + '\\llvm-strip.exe'
ld = llvmbindir + '\\lld-link.exe'
"@

Set-Content -Path $cfgpath -Value $config

# generate the build files for the project
& "python" "$datacache/meson/meson-master/meson.py" setup builddir --native-file $cfgpath

# build the project
& "ninja" -C builddir
