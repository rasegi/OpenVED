<#
.SYNOPSIS
    Build a self-contained Windows release of OpenVED and package it as a
    per-user MSI installer.

.DESCRIPTION
    Configures a Release build (MSVC + Ninja), runs the test suite, stages the
    executable together with its Qt runtime via windeployqt, and produces an MSI
    with the WiX toolset.

    Output: build\release-win\OpenVED-<version>-x64.msi

.NOTES
    Requirements on this machine (auto-detected, override via parameters):
      - Visual Studio 2022 with the C++ toolset (provides cl, cmake, ninja)
      - Qt 6 msvc2022_64 (dynamic) for windeployqt
      - vcpkg (x64-windows-static) for static FreeType/HarfBuzz
      - dotnet SDK (WiX is installed as a global dotnet tool if missing)

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\build-windows.ps1
#>
[CmdletBinding()]
param(
    [string]$QtDir      = "C:\devtools\Qt\6.9.3\msvc2022_64",
    [string]$VcpkgRoot  = "C:\devtools\vcpkg",
    [string]$Triplet    = "x64-windows-static",
    [string]$VsRoot     = "C:\Program Files\Microsoft Visual Studio\2022\Community",
    [switch]$SkipTests,
    [switch]$SkipMsi
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDir    = Join-Path $ProjectRoot "build\release-win"
$StagingDir  = Join-Path $BuildDir "staging"

# Read version straight from CMakeLists.txt so the MSI name stays in sync.
$cmakeText = Get-Content (Join-Path $ProjectRoot "CMakeLists.txt") -Raw
if ($cmakeText -match 'project\(ved_qt[\s\S]*?VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)') {
    $Version = $Matches[1]
} else {
    throw "Could not read project version from CMakeLists.txt"
}

Write-Host "==> OpenVED $Version - Windows release build" -ForegroundColor Cyan

# --- 1. Import the MSVC build environment (cl, and the paths ninja needs) -----
$vcvars = Join-Path $VsRoot "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) { throw "vcvars64.bat not found at $vcvars (set -VsRoot)" }
Write-Host "==> Importing MSVC environment from vcvars64.bat"
cmd /c "`"$vcvars`" && set" | ForEach-Object {
    if ($_ -match '^([^=]+)=(.*)$') {
        Set-Item -Path ("Env:" + $Matches[1]) -Value $Matches[2]
    }
}

# --- 2. Make sure cmake + ninja are callable (fall back to the VS-bundled ones)
function Resolve-Tool($name, $fallback) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    if (Test-Path $fallback) { return $fallback }
    throw "$name not found on PATH and no fallback at $fallback"
}
$cmakeExe = Resolve-Tool "cmake" (Join-Path $VsRoot "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe")
$ninjaDir = Join-Path $VsRoot "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
if ((-not (Get-Command ninja -ErrorAction SilentlyContinue)) -and (Test-Path $ninjaDir)) {
    $env:PATH = "$ninjaDir;$env:PATH"
}
Write-Host "    cmake: $cmakeExe"

# --- 3. Configure (Release) ---------------------------------------------------
Write-Host "==> Configure (Release)"
# Put both Qt and the vcpkg static-triplet prefix on CMAKE_PREFIX_PATH so
# find_package(Qt6) and the FreeType/HarfBuzz find_library() calls resolve
# regardless of how the vcpkg toolchain appends its own paths.
$VcpkgInstalled = Join-Path $VcpkgRoot "installed\$Triplet"
# VCPKG_MANIFEST_MODE=OFF: vcpkg.json exists in the repo root (it documents the
# FreeType/HarfBuzz deps and drives the CI build), but locally we build against
# the already-installed classic-mode vcpkg tree, so the manifest must not trigger
# a fresh manifest-mode install here.
& $cmakeExe -S $ProjectRoot -B $BuildDir -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET="$Triplet" `
    -DVCPKG_MANIFEST_MODE=OFF `
    -DVCPKG_APPLOCAL_DEPS=OFF `
    -DCMAKE_PREFIX_PATH="$QtDir;$VcpkgInstalled"
# VCPKG_APPLOCAL_DEPS=OFF: the vcpkg toolchain otherwise runs "vcpkg z-applocal"
# as a post-build step for every exe to copy DLLs next to it. With the static
# triplet there are no DLLs to copy, and on parallel CI builds these concurrent
# invocations race for the same files ("cannot access the file ... used by
# another process"). We deploy the final app's Qt runtime explicitly via
# windeployqt below, so this auto-deploy is not needed.
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

# --- 4. Build -----------------------------------------------------------------
Write-Host "==> Build"
& $cmakeExe --build $BuildDir --config Release
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

# --- 5. Tests -----------------------------------------------------------------
if (-not $SkipTests) {
    Write-Host "==> Tests"
    $ctestExe = Join-Path (Split-Path $cmakeExe) "ctest.exe"
    if (-not (Test-Path $ctestExe)) { $ctestExe = "ctest" }
    # The Qt widget test runs headless via the offscreen platform plugin, which
    # ved_copy_qt_runtime does not stage — point Qt at the plugins in the Qt
    # install so ctest can find it.
    $env:QT_QPA_PLATFORM = "offscreen"
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = Join-Path $QtDir "plugins\platforms"
    # With VCPKG_APPLOCAL_DEPS=OFF the Qt DLLs are no longer copied next to each
    # test exe, so put the Qt bin dir on PATH for the test run instead.
    $env:PATH = (Join-Path $QtDir "bin") + ";" + $env:PATH
    & $ctestExe --test-dir $BuildDir --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
}

if ($SkipMsi) { Write-Host "==> -SkipMsi set, stopping after build."; return }

# --- 6. Stage the app + Qt runtime (windeployqt) ------------------------------
Write-Host "==> Staging + windeployqt"
if (Test-Path $StagingDir) { Remove-Item -Recurse -Force $StagingDir }
New-Item -ItemType Directory -Force -Path $StagingDir | Out-Null

Copy-Item (Join-Path $BuildDir "OpenVED.exe") $StagingDir

$windeployqt = Join-Path $QtDir "bin\windeployqt.exe"
if (-not (Test-Path $windeployqt)) { throw "windeployqt not found at $windeployqt (set -QtDir)" }
& $windeployqt (Join-Path $StagingDir "OpenVED.exe") `
    --release --no-translations --no-system-d3d-compiler --no-opengl-sw
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

# App-local VC++ runtime so the installed app launches on machines without the
# system-wide redistributable (windeployqt's --compiler-runtime only bundles the
# redist *installer*, which our per-user MSI never runs).
$crtDir = Get-ChildItem (Join-Path $VsRoot "VC\Redist\MSVC") -Directory `
    | Where-Object { Test-Path (Join-Path $_.FullName "x64\Microsoft.VC143.CRT") } `
    | Sort-Object Name -Descending | Select-Object -First 1
if ($crtDir) {
    $crtSrc = Join-Path $crtDir.FullName "x64\Microsoft.VC143.CRT"
    Copy-Item "$crtSrc\vcruntime140.dll","$crtSrc\vcruntime140_1.dll","$crtSrc\msvcp140.dll" $StagingDir
    Write-Host "    + bundled VC++ runtime from $($crtDir.Name)"
} else {
    Write-Warning "VC++ runtime redist not found - installed app may need the system VC++ redistributable"
}

# windeployqt drops the ~24 MB vc_redist.x64.exe installer into the payload, but
# a per-user MSI never runs it; we bundle the loose CRT DLLs above instead.
Remove-Item (Join-Path $StagingDir "vc_redist.x64.exe") -ErrorAction SilentlyContinue

# License texts belong in every distribution (see plan Step 6b).
Copy-Item (Join-Path $ProjectRoot "THIRD_PARTY_LICENSES.md") $StagingDir
Copy-Item -Recurse (Join-Path $ProjectRoot "licenses") (Join-Path $StagingDir "licenses")

# --- 7. Build the MSI with WiX ------------------------------------------------
Write-Host "==> WiX MSI"
# Pin WiX v5: v6/v7 require accepting the paid Open Source Maintenance Fee
# (OSMF) EULA (error WIX7015). v5 is the last release usable without it and
# supports the v4 schema this .wxs targets.
$WixVersion = "5.0.2"
if (-not (Get-Command wix -ErrorAction SilentlyContinue)) {
    Write-Host "    installing WiX $WixVersion as a global dotnet tool..."
    dotnet tool install --global wix --version $WixVersion | Out-Null
    $env:PATH = "$env:USERPROFILE\.dotnet\tools;$env:PATH"
} else {
    $wixVer = (wix --version) 2>$null
    if ($wixVer -notmatch '^5\.') {
        Write-Host "    switching WiX $wixVer -> $WixVersion (avoids OSMF EULA)..."
        # 'dotnet tool update' refuses to downgrade, so reinstall.
        dotnet tool uninstall --global wix | Out-Null
        dotnet tool install --global wix --version $WixVersion | Out-Null
    }
}

$IconFile = Join-Path $ProjectRoot "packaging\icons\openved.ico"
$MsiPath  = Join-Path $BuildDir "OpenVED-$Version-x64.msi"
wix build (Join-Path $ProjectRoot "packaging\windows\openved.wxs") `
    -arch x64 `
    -d "Version=$Version" `
    -d "StagingDir=$StagingDir" `
    -d "IconFile=$IconFile" `
    -o $MsiPath
if ($LASTEXITCODE -ne 0) { throw "WiX build failed" }

Write-Host ""
Write-Host "==> Done: $MsiPath" -ForegroundColor Green
"    size: {0:N1} MB" -f ((Get-Item $MsiPath).Length / 1MB) | Write-Host
