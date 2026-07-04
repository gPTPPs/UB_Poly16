# Rebuild UB_Poly16 (Release) then compile the Windows installer.
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path

# 1. Build Release (VST3 + Standalone)
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsInstall = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$cmake = Join-Path $vsInstall 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
Write-Host "Building Release..."
& $cmake --build (Join-Path $root 'build') --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

# 2. Compile the installer (Inno Setup 6, per-user or machine-wide install)
$iscc = Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'
if (-not (Test-Path $iscc)) { $iscc = "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe" }
if (-not (Test-Path $iscc)) { throw "ISCC.exe introuvable (Inno Setup 6 requis)" }
Write-Host "Compiling installer..."
& $iscc (Join-Path $root 'installer\UB_Poly16.iss')
if ($LASTEXITCODE -ne 0) { throw "Installer compile failed" }

Write-Host "OK -> installer\Output\ (version = MyAppVersion dans UB_Poly16.iss)"
