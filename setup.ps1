function Enable-WindowsModuleInstaller-Service {
    $ServiceName = "TrustedInstaller"
    $Service = Get-Service -Name $ServiceName
    if ($Service.Status -ne "Running") {
        Write-Host "Service $ServiceName not running, enabling it now."
        Set-Service -Name $ServiceName -StartupType Automatic
        Start-Service -Name $ServiceName
    } else {
        Write-Host "Service $ServiceName is already running."
    }
}

function Install-Chocolatey {
    $HasChocolatey = (Get-Command choco -ErrorAction SilentlyContinue) -ne $null
    if ($HasChocolatey) {
        return
    }

    Write-Host "Chocolatey not found, installing it now."

    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
}

function Choco-Install {
    param(
        [string]$PackageName
    )

    $HasPackage = (& "choco" list $PackageName -e) -match $PackageName
    if ($HasPackage) {
        return
    }

    Write-Host "$PackageName not found, installing it now."
    & "choco" install $PackageName -y
}

function Install-Python {
    Choco-Install "python"
}

function Install-Python-Pip {
    $HasPip = (Get-Command pip -ErrorAction SilentlyContinue) -ne $null
    if ($HasPip) {
        return
    }

    Write-Host "Pip not found, installing it now."

    & "C:\Python313\python.exe" -m pip install pip
}

function Install-Meson {
    $HasMeson = (Get-Command meson -ErrorAction SilentlyContinue) -ne $null
    if ($HasMeson) {
        return
    }

    Write-Host "Meson not found, installing it now."

    & "C:\Python313\python.exe" -m "C:\Python313\Scripts\pip.py" install meson
}

function Install-Build-Deps {
    Choco-Install "winflexbison3"
    Choco-Install "ninja"
    Choco-Install "git"
    Choco-Install "LLVM"
    Choco-Install "cmake"
    Choco-Install "visualstudio2022buildtools"
    Choco-Install "visualstudio2022community"
}

function Install-Toolchain {
    $Cwd = Get-Location
    if (-not (Test-Path "$Cwd\llvm")) {
        & "git" clone --depth 1 https://github.com/llvm/llvm-project.git "$Cwd\llvm"
    }

    Set-Location "$Cwd\llvm"
    & "cmake" -B build -S llvm -G "Ninja" -DLLVM_ENABLE_PROJECTS="clang;lld" -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX="C:\Program Files\LLVM" -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_ASSERTIONS=OFF
    & "cmake" --build build --target install
    Set-Location $Cwd

    # Add llvm to path permanently
    $env:PATH += ";C:\Program Files\LLVM\bin"
    [Environment]::SetEnvironmentVariable("PATH", $env:PATH, [System.EnvironmentVariableTarget]::Machine)
}

function Configure-Project {
    & "C:\Python313\python.exe" setup.py build devel -Dorcl=disabled -Ddb2=disabled -Dpsql=disabled -Dmysql=disabled --backend=vs2022
}

Enable-WindowsModuleInstaller-Service
Install-Chocolatey
Install-Python
Install-Python-Pip
Install-Build-Deps
Install-Meson
Install-Toolchain

Import-Module "$env:ProgramFiles\Microsoft Visual Studio\2022\Community\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Enter-VsDevShell -VsInstallPath "$env:ProgramFiles\Microsoft Visual Studio\2022\Community" -DevCmdArguments '-arch=x64'

Configure-Project
