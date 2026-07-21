# Plan: OpenVED Installer-Build und GitHub-Distribution

Datum: 2026-05-27
Status: offen

## Kontext

OpenVED ist eine Qt6 C++23 Desktop-Anwendung (CMake-Projekt `ved_qt`, Version 0.1.0).
GitHub-Repo existiert: `https://github.com/rasegi/OpenVED.git`

Aktuell fehlt: App-Icons, Info.plist, install()-Regeln, Build-Skripte, CI/CD, Installer.

Ziel: DMG fuer macOS Apple Silicon und MSI fuer Windows x64 — sowohl lokal baubar
als auch automatisch ueber GitHub Actions bei Release-Tags.

Code-Signing: spaeter, aber Hook-Points werden vorbereitet.

## Bestandsaufnahme

### Was existiert
- `CMakeLists.txt` mit `MACOSX_BUNDLE TRUE`, `WIN32_EXECUTABLE TRUE`
- Qt6 Modules: Widgets, PrintSupport, Gui
- FreeType2 + HarfBuzz (statisch bevorzugt, Vendor-Fallback unter `third_party/`)
- Ressourcen eingebettet via `qt_add_resources` (Toolbar-Icons, Cursor, Fonts)
- `ved_copy_qt_runtime()` fuer Windows-DLL-Kopie (Debug-Builds)
- MSVC/vcpkg-Unterstuetzung im CMakeLists.txt vorhanden

### Was fehlt
- Kein `.github/` Verzeichnis (kein CI/CD)
- Kein App-Icon (.icns / .ico)
- Kein custom Info.plist
- Keine `install()` Regeln in CMake
- Keine Build-Skripte fuer Release
- Kein `vcpkg.json` Manifest
- Kein `packaging/` Verzeichnis

### Lokale Umgebung
- macOS Apple Silicon mit Homebrew Qt 6.9.x, `macdeployqt` unter `/opt/homebrew/bin/`
- Windows-Maschine mit Visual Studio vorhanden
- Git Remote: `https://github.com/rasegi/OpenVED.git`

---

## Step 1: Packaging-Grundlagen in CMake

**umgesetzt am 2026-07-21** (Branch `plan_installer_build_githun_distribution`):
- Platzhalter-App-Icon `packaging/icons/openved-1024.png` ("Open" oben, "VED"
  unten, blauer Verlauf) → `openved.icns` (macOS) + `openved.ico` (Windows);
  Fenster-Icon als Qt-Ressource `src/app/resources/openved.png` (256px).
- `packaging/windows/openved.rc`, `packaging/macos/Info.plist.in`
  (CFBundleName/Identifier/Version/Icon/Category, NSHighResolutionCapable).
- CMake: Bundle-Properties auf `ved_qt_app`, Icon-Einbindung (macOS-Resource via
  `MACOSX_PACKAGE_LOCATION`, Windows-RC), `install(TARGETS ... BUNDLE/RUNTIME)`.
- `src/app/main.cpp`: `setWindowIcon(:/ved/openved.png)`.
- Verifiziert: Build gruen; `.icns` + `Info.plist` (CFBundleName "OpenVED") im
  Bundle; `cmake --install` erzeugt vollstaendiges `.app`; Smoke-Test stabil.
- Offen fuer Step 2: App-Bundle heisst noch `ved_qt_app.app` (OUTPUT_NAME
  "OpenVED" bewusst zurueckgestellt, um Build/Skripte konsistent umzustellen).

### 1a: App-Icon erstellen

**Neue Dateien:**
```
packaging/
  icons/
    openved-1024.png     # Quell-PNG (1024x1024, Platzhalter)
    openved.icns         # macOS (generiert via iconutil)
    openved.ico          # Windows (generiert via ImageMagick oder online-Tool)
```

macOS-Icon generieren:
```bash
mkdir openved.iconset
sips -z 16 16   openved-1024.png --out openved.iconset/icon_16x16.png
sips -z 32 32   openved-1024.png --out openved.iconset/icon_16x16@2x.png
sips -z 32 32   openved-1024.png --out openved.iconset/icon_32x32.png
sips -z 64 64   openved-1024.png --out openved.iconset/icon_32x32@2x.png
sips -z 128 128 openved-1024.png --out openved.iconset/icon_128x128.png
sips -z 256 256 openved-1024.png --out openved.iconset/icon_128x128@2x.png
sips -z 256 256 openved-1024.png --out openved.iconset/icon_256x256.png
sips -z 512 512 openved-1024.png --out openved.iconset/icon_256x256@2x.png
sips -z 512 512 openved-1024.png --out openved.iconset/icon_512x512.png
sips -z 1024 1024 openved-1024.png --out openved.iconset/icon_512x512@2x.png
iconutil -c icns openved.iconset -o openved.icns
```

Windows-Icon: `convert openved-1024.png -define icon:auto-resize=256,128,64,48,32,16 openved.ico`

### 1b: Windows Resource-Datei

**Neue Datei: `packaging/windows/openved.rc`**
```rc
IDI_ICON1 ICON "openved.ico"
```

### 1c: macOS Info.plist Template

**Neue Datei: `packaging/macos/Info.plist.in`**

Basiert auf Qt-Default-Template, ergaenzt um:
- `CFBundleIdentifier`: `com.ved.openved`
- `CFBundleIconFile`: `openved.icns`
- `NSHighResolutionCapable`: `true`
- CMake-Variablen fuer Version: `${MACOSX_BUNDLE_BUNDLE_VERSION}`

### 1d: CMakeLists.txt aendern

Bundle-Properties auf `ved_qt_app` setzen:
```cmake
set_target_properties(ved_qt_app PROPERTIES
    MACOSX_BUNDLE_BUNDLE_NAME "OpenVED"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.ved.openved"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "Copyright 2026 Lukas Rasegi"
    MACOSX_BUNDLE_ICON_FILE "openved.icns"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/packaging/macos/Info.plist.in"
)
```

macOS Icon als Bundle-Ressource:
```cmake
if(APPLE)
    set(MACOSX_ICON "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icons/openved.icns")
    set_source_files_properties(${MACOSX_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(ved_qt_app PRIVATE ${MACOSX_ICON})
endif()
```

Windows RC einbinden:
```cmake
if(WIN32)
    configure_file(packaging/windows/openved.rc "${CMAKE_CURRENT_BINARY_DIR}/openved.rc" COPYONLY)
    file(COPY packaging/icons/openved.ico DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    target_sources(ved_qt_app PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/openved.rc")
endif()
```

Install-Regel:
```cmake
include(GNUInstallDirs)
install(TARGETS ved_qt_app
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
```

### 1e: Window-Icon in main.cpp

Nach `QApplication`-Konstruktion:
```cpp
app.setWindowIcon(QIcon(QStringLiteral(":/ved/toolbar/new_drawing.bmp")));
```
Spaeter durch eigenes App-Icon in Qt-Ressourcen ersetzen.

---

## Step 2: Lokale Build-Skripte

### 2a: macOS — `scripts/build-macos.sh`

**umgesetzt am 2026-07-22** (Branch `plan_installer_build_githun_distribution`):
- `scripts/build-macos.sh` erstellt ein self-contained, ad-hoc signiertes DMG
  (`OpenVED-0.1.0-macOS-arm64.dmg`, ~29 MB). Reproduzierbar getestet: frischer
  `rm -rf build/release` → Configure → Build → 27/27 Tests → Bundle → Smoke-Test
  → DMG; App aus dem gemounteten DMG heraus gestartet und stabil.
- `OUTPUT_NAME "OpenVED"` ist jetzt aktiv (Bundle heisst `OpenVED.app`).
- Tests laufen headless via `QT_QPA_PLATFORM=offscreen`.
- **Homebrew-Qt-Fixup (zentrale Erkenntnis):** `macdeployqt` liefert bei
  Homebrew-Qt kein vollstaendiges Bundle:
  1. Qt-Frameworks liegen unter der `qtbase`-Keg, nicht auf dem Default-Suchpfad
     → `-libpath="$(brew --prefix qtbase)/lib"`.
  2. `macdeployqt` schreibt `QtGui → @rpath/QtDBus` um, **kopiert QtDBus aber
     nicht** ins Bundle → Absturz beim Start (`Library not loaded:
     @rpath/QtDBus.framework`). Ausserdem fehlen transitive Homebrew-`.dylib`s
     (`libdbus-1.3`).
  → Post-`macdeployqt`-Fixup schliesst die Abhaengigkeits-Huelle iterativ:
  fehlende `@rpath`-Qt-Frameworks nachziehen, absolute Homebrew-Qt-Referenzen
  auf `@rpath` umbiegen, transitive `.dylib`s einbetten, System-/Homebrew-rpaths
  entfernen (verhindert Doppel-Load → `class implemented in both`), ad-hoc
  signieren (arm64 verweigert sonst invalidierte Signatur), Smoke-Test.
- **Hinweis:** In der CI (Step 3) wird **offizielles Qt** via
  `install-qt-action` genutzt; dort erzeugt `macdeployqt` bereits ein
  vollstaendiges Bundle und der Fixup ist ein harmloser No-Op.
- Offen: Code-Signing/Notarization mit echtem Developer-ID-Zertifikat (Hook
  vorbereitet: `-` durch `Developer ID Application: …` ersetzen); `create-dmg`
  fuer Drag-to-Applications-UI (aktuell schlichtes `hdiutil`-DMG).

Der urspruenglich geplante Skelett-Code unten diente als Ausgangspunkt; das
tatsaechliche Skript enthaelt zusaetzlich den oben beschriebenen Fixup.

```bash
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build/release"
VERSION=$(grep 'project(ved_qt' "${PROJECT_ROOT}/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')

# 1. Configure
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix freetype);$(brew --prefix harfbuzz)" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
    -DCMAKE_OSX_ARCHITECTURES=arm64

# 2. Build
cmake --build "$BUILD_DIR" --config Release

# 3. Tests
ctest --test-dir "$BUILD_DIR" --output-on-failure

# 4. macdeployqt
macdeployqt "$BUILD_DIR/ved_qt_app.app" -verbose=1
# Code-Signing spaeter: -codesign="Developer ID Application: ..."

# 5. DMG erstellen
DMG_NAME="OpenVED-${VERSION}-macOS-arm64.dmg"
hdiutil create -volname "OpenVED" \
    -srcfolder "$BUILD_DIR/ved_qt_app.app" \
    -ov -format UDZO \
    "$BUILD_DIR/$DMG_NAME"

echo "DMG erstellt: $BUILD_DIR/$DMG_NAME"
```

Optional: `create-dmg` statt `hdiutil` fuer Drag-to-Applications UI:
```bash
brew install create-dmg
create-dmg \
    --volname "OpenVED" \
    --window-pos 200 120 --window-size 600 400 \
    --icon-size 100 \
    --icon "ved_qt_app.app" 150 190 \
    --app-drop-link 450 190 \
    "$BUILD_DIR/$DMG_NAME" "$BUILD_DIR/ved_qt_app.app"
```

### 2b/2c: Windows — `scripts/build-windows.ps1` + `packaging/windows/openved.wxs`

**umgesetzt am 2026-07-22** (Branch `plan_installer_build_githun_distribution`):
- `scripts/build-windows.ps1` baut lokal ein per-user MSI
  (`OpenVED-0.1.0-x64.msi`, ~18,7 MB). Reproduzierbar getestet: sauberer
  `build/release-win` → Configure → Build → **27/27 Tests** → windeployqt-Staging
  → MSI; danach still installiert (`msiexec /qn`, exit 0, 50,5 MB in
  `%LOCALAPPDATA%\OpenVED`), Startmenü-Verknüpfung angelegt, installierte
  `OpenVED.exe` gestartet und stabil (Fenstertitel „VED Qt Port").
- **Toolchain dieser Maschine:** MSVC 2022, Qt `C:\devtools\Qt\6.9.3\msvc2022_64`
  (dynamisch → windeployqt), vcpkg `C:\devtools\vcpkg` Triplet
  `x64-windows-static` (FreeType/HarfBuzz statisch), cmake/ninja VS-gebündelt.
  Das Skript importiert `vcvars64.bat` und findet cmake/ninja per Fallback.
- **Zentrale Erkenntnisse:**
  1. `VCPKG_TARGET_TRIPLET` **muss** in PowerShell quotiert übergeben werden
     (`-DVCPKG_TARGET_TRIPLET="$Triplet"`) — unquotiert landete literal
     `$Triplet` im Cache → kaputte vcpkg-Config-Pfade.
  2. `CMAKE_PREFIX_PATH` explizit auf Qt **und** den vcpkg-installed-Prefix
     setzen, damit FreeType-CONFIG (zieht zlib/png/brotli/bzip2 transitiv) +
     Qt sauber auflösen.
  3. Der Qt-Widget-Test braucht das **offscreen**-Plugin, das
     `ved_copy_qt_runtime` nicht staged → `QT_QPA_PLATFORM_PLUGIN_PATH` auf
     `<Qt>\plugins\platforms` für ctest.
  4. **WiX v5 gepinnt** (`dotnet tool install --global wix --version 5.0.2`):
     WiX v6/v7 verlangen die kostenpflichtige OSMF-EULA (Fehler WIX7015).
     Downgrade braucht uninstall+install (nicht `tool update`).
  5. VC++-Runtime **app-lokal** gebündelt (`vcruntime140*.dll`, `msvcp140.dll`
     aus dem VS-Redist) statt windeployqts `--compiler-runtime` (das nur den
     24-MB-`vc_redist.x64.exe`-Installer beilegt, den ein per-user-MSI nie
     ausführt) — Installer dadurch von 42,9 auf 18,7 MB geschrumpft.
- `packaging/windows/openved.wxs`: WiX-4-Schema, `Scope="perUser"` →
  `%LOCALAPPDATA%\OpenVED`, Payload rekursiv per `<Files Include="…\**">` aus
  dem Staging-Verzeichnis geerntet (Qt-DLLs + Plugin-Ordner + `licenses/`),
  Startmenü-Shortcut, `MajorUpgrade`, ARP-Icon.
- **Offen:** `dxcompiler.dll`/`dxil.dll` (~15 MB, D3D-Shader-Compiler) noch im
  Payload — für die reine Widgets-App verzichtbar, könnte getrimmt werden;
  Code-Signing (`signtool`); `vcpkg.json` (2d) bewusst zurückgestellt.

Der ursprünglich geplante Skelett-Code unten diente als Ausgangspunkt.

### 2b: Windows — `scripts/build-windows.ps1`

```powershell
param(
    [string]$QtDir = "",
    [string]$VcpkgRoot = "C:\vcpkg",
    [switch]$SkipMsi
)
$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $ProjectRoot "build\release"
$Version = "0.1.0"

# 1. Configure
cmake -S $ProjectRoot -B $BuildDir -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgRoot\scripts\buildsystems\vcpkg.cmake" `
    -DVCPKG_TARGET_TRIPLET=x64-windows

# 2. Build
cmake --build $BuildDir --config Release

# 3. Tests
ctest --test-dir $BuildDir --output-on-failure

# 4. windeployqt
$ExePath = Join-Path $BuildDir "ved_qt_app.exe"
windeployqt $ExePath --release --no-translations --no-system-d3d-compiler --no-opengl-sw

# 5. MSI erstellen (WiX 4)
if (-not $SkipMsi) {
    dotnet tool install --global wix 2>$null
    $StagingDir = Join-Path $BuildDir "staging"
    New-Item -ItemType Directory -Force -Path $StagingDir
    Copy-Item "$BuildDir\ved_qt_app.exe" $StagingDir
    Copy-Item "$BuildDir\*.dll" $StagingDir
    Copy-Item -Recurse "$BuildDir\platforms" "$StagingDir\platforms" -ErrorAction SilentlyContinue
    Copy-Item -Recurse "$BuildDir\styles" "$StagingDir\styles" -ErrorAction SilentlyContinue
    Copy-Item -Recurse "$BuildDir\imageformats" "$StagingDir\imageformats" -ErrorAction SilentlyContinue

    wix build "$ProjectRoot\packaging\windows\openved.wxs" `
        -d "Version=$Version" `
        -d "SourceDir=$StagingDir" `
        -o "$BuildDir\OpenVED-$Version-x64.msi"
    Write-Host "MSI erstellt: $BuildDir\OpenVED-$Version-x64.msi"
}
```

### 2c: WiX 4 Installer-Definition

**Neue Datei: `packaging/windows/openved.wxs`**

PerUser-Installation unter `%LOCALAPPDATA%\OpenVED`. DLLs werden via `wix heat dir`
aus dem Staging-Verzeichnis automatisch erfasst.

### 2d: vcpkg Manifest

**umgesetzt am 2026-07-22** (Branch `plan_installer_build_githun_distribution`):
- `vcpkg.json` angelegt — **abweichend vom Entwurf ohne `qtbase`**, nur
  `freetype` + `harfbuzz`. Begruendung: Qt kommt in diesem Projekt **extern**
  (lokal aus `C:\devtools\Qt`, in CI ueber `install-qt-action`); `qtbase` im
  Manifest wuerde vcpkg zwingen, Qt **aus dem Quellcode** zu bauen (Stunden) und
  mit der externen Qt kollidieren. Das Manifest deckt daher nur ab, was vcpkg
  hier tatsaechlich liefert.
- **Lokaler Build bleibt Classic-Mode:** `build-windows.ps1` uebergibt
  `-DVCPKG_MANIFEST_MODE=OFF` und baut gegen den vorinstallierten
  `C:\devtools\vcpkg\installed\x64-windows-static`-Baum. Die blosse Existenz der
  `vcpkg.json` wuerde vcpkg sonst in den Manifest-Mode kippen (frischer
  FreeType/HarfBuzz-Rebuild). Verifiziert: sauberer Rebuild mit vorhandener
  `vcpkg.json` weiterhin gruen, MSI 18,6 MB.
- Das Manifest ist damit die **dokumentierte Dependency-Quelle** und dient dem
  CI-Build (Step 3, Manifest-Mode via `run-vcpkg`); lokal wird es bewusst
  umgangen.

Urspruenglicher Entwurf (mit `qtbase` — verworfen, s.o.):
```json
{
    "name": "openved",
    "version": "0.1.0",
    "dependencies": [
        { "name": "qtbase", "default-features": false, "features": ["widgets", "printsupport"] },
        "freetype",
        "harfbuzz"
    ]
}
```

---

## Step 3: GitHub Actions Workflow

**Neue Datei: `.github/workflows/release.yml`** — **umgesetzt am 2026-07-22**
(YAML validiert; **CI-Lauf ausstehend** — nur auf GitHub testbar):
- 3 Jobs: `build-macos` (macos-14), `build-windows` (windows-latest),
  `release` (ubuntu, `needs` beide, nur bei Tag `v*`).
- **Strategie: CI ruft die getesteten lokalen Skripte auf** (statt Build-Logik
  zu duplizieren):
  - macOS: `brew install qt freetype harfbuzz ninja` → `scripts/build-macos.sh`
    (Homebrew-Fixup laeuft mit — identisch zum lokalen Build).
  - Windows: `install-qt-action` (offizielles Qt) + `vcpkg install freetype
    harfbuzz --triplet x64-windows-static` (classic-mode, **im vcpkg-Root**
    ausgefuehrt, damit `vcpkg.json` nicht den Manifest-Mode ausloest) →
    `scripts/build-windows.ps1` mit `-QtDir $QT_ROOT_DIR`,
    `-VcpkgRoot $VCPKG_INSTALLATION_ROOT`, `-VsRoot` (via `vswhere`),
    `-Triplet x64-windows-static`.
- Trigger: Tag-Push `v*` (→ Release) **und** `workflow_dispatch` (Testlauf, nur
  Artefakte, kein Release — `release`-Job hat `if: startsWith(...refs/tags/v)`).
- Pre-Release automatisch bei Tags mit Bindestrich (`v0.2.0-beta.1`).
- **Hinweis:** Erster CI-Lauf braucht erfahrungsgemaess ein paar Runden
  (VS-Pfad, Qt-arch-String, vcpkg/WiX in der CI-Umgebung) — normale Anlauf-
  Reibung, kein Homebrew-Kaninchenbau.

Trigger: Tag-Push `v*` (z.B. `v0.1.0`)

### Job 1: build-macos (macos-14 = Apple Silicon)
1. Checkout
2. Qt installieren via `jurplel/install-qt-action@v4` (Qt 6.8.3 LTS, cached)
3. `brew install ninja freetype harfbuzz create-dmg`
4. CMake Configure + Build
5. `ctest` (QT_QPA_PLATFORM=offscreen)
6. `macdeployqt`
7. DMG via `create-dmg`
8. Upload Artifact

### Job 2: build-windows (windows-latest)
1. Checkout
2. Qt installieren via `install-qt-action` (win64_msvc2022_64)
3. vcpkg Setup via `lukka/run-vcpkg@v11`
4. `vcpkg install freetype harfbuzz --triplet x64-windows`
5. CMake Configure + Build
6. `ctest`
7. `windeployqt`
8. MSI via WiX 4
9. Upload Artifact

### Job 3: create-release (ubuntu-latest, needs: build-macos + build-windows)
1. Download beide Artifacts
2. GitHub Release erstellen via `softprops/action-gh-release@v2`
3. DMG + MSI als Release-Assets
4. Pre-Release wenn Tag `-alpha`, `-beta` etc. enthaelt

---

## Step 5: WebAssembly Web-Distribution

Dritter Distributionspfad neben DMG/MSI: OpenVED als statisch gehostetes
Web-Bundle (GitHub Pages) ueber Qt for WebAssembly.

**Abhaengigkeit:** Dieser Step 5 (der WASM-Teil des Plans) ist von
`story_16_webassembly.md` abhaengig — die dort beschriebene
Anwendungs-Portierung (PrintSupport-Entkopplung, async Datei-I/O,
`.vfn`-Font-Bundle, lokaler Font-Server) muss vorliegen, bevor ein
sinnvolles Bundle gebaut/deployt werden kann. Story 16 haengt ihrerseits an
`story_15_font_converter_tool.md`. Hier im Plan nur das Build-/Deployment-Setup.

### 5a: `scripts/build-wasm.sh`

```bash
#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build/wasm"
DIST_DIR="${BUILD_DIR}/dist"

# Voraussetzung: emsdk aktiviert + Qt for WebAssembly (wasm_singlethread)
# source "$EMSDK/emsdk_env.sh"
QT_WASM="${QT_WASM:-$HOME/Qt/6.8.3/wasm_singlethread}"

# 1. Configure (Qt-WASM-Toolchain bindet Emscripten intern ein)
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$QT_WASM/lib/cmake/Qt6/qt.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release

# 2. Build
cmake --build "$BUILD_DIR"

# 3. Bundle sammeln (.html/.js/.wasm/qtloader.js)
mkdir -p "$DIST_DIR"
cp "$BUILD_DIR"/ved_qt_app.{html,js,wasm} "$DIST_DIR/" 2>/dev/null || true
cp "$BUILD_DIR"/qtloader.js "$DIST_DIR/" 2>/dev/null || true
# index.html = umbenanntes App-HTML fuer Pages-Root
cp "$DIST_DIR/ved_qt_app.html" "$DIST_DIR/index.html"

echo "WASM-Bundle: $DIST_DIR"
```

### 5b: GitHub-Actions-Job `build-wasm` (ubuntu-latest)

Ergaenzung in `.github/workflows/release.yml`:

1. Checkout
2. emsdk installieren via `mymindstorm/setup-emsdk@v14` (Version passend zur
   Qt-Version festnageln)
3. Qt for WebAssembly installieren via `jurplel/install-qt-action@v4`
   (`arch: wasm_singlethread`, plus Desktop-Qt als Host-Tool)
4. `scripts/build-wasm.sh`
5. Bundle als Artifact hochladen **und** via `actions/upload-pages-artifact` +
   `actions/deploy-pages` auf GitHub Pages veroeffentlichen
   (eigener Job mit `permissions: pages: write, id-token: write`)

Trigger: zusaetzlich zum Tag-Push auch manuell (`workflow_dispatch`), damit die
Web-Version unabhaengig von Desktop-Releases aktualisiert werden kann.

### 5c: CMake-Anpassung

- MSVC-/Windows-Kit- und `ved_copy_qt_runtime()`-Bloecke hinter
  `if(NOT EMSCRIPTEN)` / `if(WIN32)` gaten.
- FreeType/HarfBuzz unter Emscripten ueber Emscripten-Ports / Qt-mitgeliefertes
  FreeType beziehen (find-/vendor-Logik nur im nativen Zweig).
- `Qt6::PrintSupport` nur im nativen Zweig linken.

---

## Step 6: Lizenz-Compliance (alle Distributionspfade)

Gilt fuer DMG, MSI **und** WASM. Ausgangslage: `README.md` behauptet
"uses Qt under the LGPL ... See LICENSE for details", aber `LICENSE` enthielt
nur den MIT-Text — die Third-Party-Lizenztexte fehlten. Geschlossen durch:

### 6a: Lizenztexte im Repo

`licenses/` mit den vollstaendigen Texten (bereits eingesammelt):
```
licenses/
  Qt-LGPL-3.0.txt         # Qt (Widgets/Gui/PrintSupport)
  Qt-GPL-3.0.txt          # von LGPLv3 referenziert
  FreeType-FTL.txt        # FreeType (FTL)
  HarfBuzz-COPYING.txt    # HarfBuzz (Old MIT)
  Liberation-OFL-1.1.txt  # gebuendelte Fonts (OFL 1.1)
  DejaVu-LICENSE.txt      # optional, falls DejaVu gebuendelt wird
```
`THIRD_PARTY_LICENSES.md` (Repo-Root) fasst Komponenten, Lizenzen, Link-Art
und Pflicht-Credits zusammen (u.a. FreeType-FTL-Credit).

### 6b: Lizenzen in jede Distribution packen

- **macOS DMG:** `licenses/` + `THIRD_PARTY_LICENSES.md` ins App-Bundle
  (`Contents/Resources/licenses/`) via `install()`/`MACOSX_PACKAGE_LOCATION`.
- **Windows MSI:** `licenses/` ins Staging-Verzeichnis aufnehmen (WiX-Komponente).
- **WASM:** `licenses/` + `THIRD_PARTY_LICENSES.md` in `build/wasm/dist/`
  kopieren; in der App ein "About / Licenses"-Dialog oder Link darauf.

### 6c: LGPL-Besonderheiten

- **Nativ:** Qt dynamisch gelinkt (macdeployqt/windeployqt) → LGPLv3-Austausch-
  barkeit erfuellt.
- **WASM:** Qt **statisch** gelinkt. LGPLv3 verlangt Relink-Faehigkeit → wird
  dadurch erfuellt, dass OpenVEDs Quellcode + Build-Anleitung oeffentlich sind
  (MIT/GitHub). Muss fuer jedes veroeffentlichte WASM-Release gelten.
- FreeType (FTL) und HarfBuzz (MIT) erlauben statisches Linken ohne Copyleft;
  nur Lizenztext + FreeType-Credit noetig.

### 6d: README korrigieren

`README.md`-Lizenzabschnitt auf `THIRD_PARTY_LICENSES.md` + `licenses/`
verweisen lassen (statt nur auf `LICENSE`).

---

## Step 4: Release-Prozess

1. Version bumpen in `CMakeLists.txt`: `project(ved_qt VERSION X.Y.Z ...)`
2. Committen: `git commit -am "Release vX.Y.Z"`
3. Taggen: `git tag vX.Y.Z`
4. Pushen: `git push origin main --tags`
5. GitHub Actions baut automatisch → Release mit DMG + MSI erscheint

### Naming Convention
- Tags: `v0.1.0`, `v0.2.0`, `v1.0.0`
- Pre-Releases: `v0.2.0-alpha.1`
- Assets: `OpenVED-0.1.0-macOS-arm64.dmg`, `OpenVED-0.1.0-x64.msi`

### README Badge
```markdown
[![Release](https://img.shields.io/github/v/release/rasegi/OpenVED)](https://github.com/rasegi/OpenVED/releases/latest)
```

---

## Neue Dateistruktur (Zusammenfassung)

```
packaging/
  icons/openved-1024.png, openved.icns, openved.ico
  macos/Info.plist.in
  windows/openved.rc, openved.wxs
scripts/
  build-macos.sh
  build-windows.ps1
  build-wasm.sh
vcpkg.json
.github/workflows/release.yml   # + build-wasm-Job + Pages-Deploy
```

Hinweis: Der WebAssembly-Pfad (Step 5) und die dafuer noetige App-Portierung
(PrintSupport-Entkopplung, async Datei-I/O, `.vfn`-Font-Bundle, lokaler
Font-Server) sind in `story_16_webassembly.md` beschrieben; das Font-Bundle
setzt das CLI aus `story_15_font_converter_tool.md` voraus.

## Geaenderte Dateien
- `CMakeLists.txt` — Icon-Handling, Bundle-Properties, install(), RC-Einbindung
- `src/app/main.cpp` — `setWindowIcon()`
- `.gitignore` — `build/release/`, `*.dmg`, `*.msi`

---

## Code-Signing (spaeter)

Hook-Points im Plan vorbereitet:
- **macOS**: `macdeployqt -codesign=...` + `xcrun notarytool submit`
- **Windows**: `signtool sign /fd SHA256 ...` nach MSI-Erstellung
- CI-Secrets: `APPLE_CERTIFICATE_BASE64`, `WINDOWS_CERTIFICATE_BASE64` etc.

---

## Umsetzungsreihenfolge

1. `packaging/` Verzeichnis + Platzhalter-Icons
2. `packaging/macos/Info.plist.in` + `packaging/windows/openved.rc`
3. `CMakeLists.txt` anpassen (Icons, Bundle, install)
4. `main.cpp` — `setWindowIcon()`
5. `vcpkg.json`
6. `scripts/build-macos.sh` — lokal testen
7. `packaging/windows/openved.wxs` + `scripts/build-windows.ps1` — lokal auf Windows testen
8. `.github/workflows/release.yml`
9. Tag pushen, CI-Pipeline pruefen
10. `README.md` mit Badge und Download-Links
