# Build UB_Poly16 (VST3 + Standalone) with the VS Build Tools CMake.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmake = Join-Path $vsInstall 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'

Write-Host "CMake: $cmake"
Write-Host "Configuring..."
# Local build: ASIO on (the SDK lives outside the repo and is never committed)
& $cmake -S $root -B (Join-Path $root 'build') -G "Visual Studio 17 2022" -A x64 `
    -DUB_ASIO=ON `
    -DUB_ASIO_SDK_DIR="D:/Audio/asiosdk_extract/asiosdk_2.3.3_2019-06-14/common"
if ($LASTEXITCODE -ne 0) { throw "Configure failed" }

Write-Host "Building (Release)..."
& $cmake --build (Join-Path $root 'build') --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Host "Done."
