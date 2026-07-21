#!/usr/bin/env bash
#
# Build a self-contained macOS release of OpenVED and package it as a DMG.
#
#   scripts/build-macos.sh
#
# Requirements: Homebrew Qt 6, freetype, harfbuzz, ninja, and macdeployqt on PATH.
# Output: build/release/OpenVED-<version>-macOS-arm64.dmg
#
# Note on Homebrew Qt: Homebrew ships the Qt frameworks under the "qtbase" keg,
# which is not on macdeployqt's default search path, and some Qt frameworks
# (QtDBus, ...) link transitive Homebrew dylibs (libdbus-1.3) that macdeployqt
# does not embed. The post-deploy fixup below makes the bundle truly
# self-contained. On official Qt (as used in CI) macdeployqt already produces a
# complete bundle and the fixup is a harmless no-op.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build/release"
APP="OpenVED.app"
APP_PATH="${BUILD_DIR}/${APP}"
BIN="${APP_PATH}/Contents/MacOS/OpenVED"
FW="${APP_PATH}/Contents/Frameworks"
VERSION="$(grep -A2 'project(ved_qt' "${PROJECT_ROOT}/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)"
DMG_NAME="OpenVED-${VERSION}-macOS-arm64.dmg"

echo "==> OpenVED ${VERSION} — macOS release build"

# 1. Configure (Release)
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix freetype);$(brew --prefix harfbuzz)" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 \
    -DCMAKE_OSX_ARCHITECTURES=arm64

# 2. Build
cmake --build "$BUILD_DIR" --config Release

# 3. Tests (offscreen so the Qt widget tests run headless)
QT_QPA_PLATFORM=offscreen ctest --test-dir "$BUILD_DIR" --output-on-failure

# 4. Bundle Qt frameworks into the .app (self-contained)
#    Homebrew installs the Qt frameworks under the qtbase keg, which is not on
#    macdeployqt's default search path — point it there explicitly via -libpath.
#    Code-signing later: add  -codesign="Developer ID Application: ..."
QT_FRAMEWORK_PATH="$(brew --prefix qtbase)/lib"
macdeployqt "$APP_PATH" -verbose=1 -libpath="$QT_FRAMEWORK_PATH"

# 5. Post-deploy fixup — make the bundle truly self-contained (Homebrew Qt).
echo "==> Post-deploy fixup (embedding transitive Homebrew dependencies)"

# 5a. Ensure the executable resolves embedded frameworks via @rpath, and drop
#     any rpath pointing back at system/Homebrew Qt (avoids the same library
#     being loaded twice → "class implemented in both" abort).
install_name_tool -add_rpath "@executable_path/../Frameworks" "$BIN" 2>/dev/null || true
for bad in "$QT_FRAMEWORK_PATH" "$(brew --prefix)/lib"; do
    install_name_tool -delete_rpath "$bad" "$BIN" 2>/dev/null || true
done

# 5b. Complete the dependency closure. macdeployqt sometimes rewrites a Qt
#     framework reference to @rpath (e.g. QtGui → @rpath/QtDBus) without actually
#     copying that framework, and it leaves transitive Homebrew dylibs
#     (libdbus-1.3, ...) behind. Iterate until the bundle is self-contained:
#       - copy any @rpath Qt framework that is referenced but missing,
#       - copy any Qt framework still referenced by absolute Homebrew path,
#       - copy any Homebrew dylib, rewriting the reference to @rpath.
#     The glob is re-evaluated each round, so newly copied frameworks get their
#     own dependencies resolved on the next pass.
HB_PREFIX="$(brew --prefix)"
copy_framework() {  # $1 = framework name (e.g. QtDBus)
    local fwname="$1"
    [ -d "$FW/$fwname.framework" ] && return 1
    [ -d "$QT_FRAMEWORK_PATH/$fwname.framework" ] || return 1
    cp -R "$QT_FRAMEWORK_PATH/$fwname.framework" "$FW/$fwname.framework"
    rm -rf "$FW/$fwname.framework/Headers" \
           "$FW/$fwname.framework/Versions/A/Headers" 2>/dev/null || true
    find "$FW/$fwname.framework" -name '*.prl' -delete 2>/dev/null || true
    install_name_tool -id "@rpath/$fwname.framework/Versions/A/$fwname" \
        "$FW/$fwname.framework/Versions/A/$fwname" 2>/dev/null || true
    echo "    + embedded framework $fwname"
    return 0
}
for _ in 1 2 3 4 5 6 7 8; do
    changed=0
    while IFS= read -r bin; do
        [ -f "$bin" ] || continue
        # (i) missing @rpath Qt frameworks
        while IFS= read -r ref; do
            [ -n "$ref" ] || continue
            copy_framework "$(basename "$ref")" && changed=1
        done < <(otool -L "$bin" 2>/dev/null | grep -oE "@rpath/Qt[A-Za-z]+\.framework/Versions/[A-Z]+/Qt[A-Za-z]+" | sed -E 's|.*/([A-Za-z]+)$|\1|' || true)
        # (ii) Qt frameworks still referenced by absolute Homebrew path
        while IFS= read -r ref; do
            [ -n "$ref" ] || continue
            fwname="$(basename "$ref")"
            copy_framework "$fwname" && changed=1
            install_name_tool -change "$ref" "@rpath/$fwname.framework/Versions/A/$fwname" "$bin" 2>/dev/null || true
        done < <(otool -L "$bin" 2>/dev/null | grep -oE "${HB_PREFIX}[^ ]*/Qt[A-Za-z]+\.framework/Versions/[A-Z]+/Qt[A-Za-z]+" || true)
        # (iii) Homebrew dylibs
        while IFS= read -r dep; do
            [ -n "$dep" ] || continue
            depname="$(basename "$dep")"
            if [ ! -f "$FW/$depname" ]; then
                cp "$dep" "$FW/$depname"
                chmod u+w "$FW/$depname"
                install_name_tool -id "@rpath/$depname" "$FW/$depname" 2>/dev/null || true
                echo "    + embedded dylib $depname"
                changed=1
            fi
            install_name_tool -change "$dep" "@rpath/$depname" "$bin" 2>/dev/null || true
        done < <(otool -L "$bin" 2>/dev/null | grep -oE "${HB_PREFIX}[^ ]*\.dylib" || true)
    done < <(printf '%s\n' "$BIN" "$FW"/*.dylib "$FW"/*.framework/Versions/A/*)
    [ "$changed" = "0" ] && break
done

# 5c. Normalise Qt framework self-ids to @rpath.
for fwbin in "$FW"/*.framework/Versions/A/*; do
    [ -f "$fwbin" ] || continue
    name="$(basename "$fwbin")"
    case "$name" in
        Qt*) install_name_tool -id "@rpath/${name}.framework/Versions/A/${name}" "$fwbin" 2>/dev/null || true ;;
    esac
done

# 5d. Verify nothing still points at Homebrew before signing.
LEAKS="$(find "$APP_PATH" -type f -exec sh -c 'otool -L "$1" 2>/dev/null | grep -q "'"$HB_PREFIX"'/opt" && basename "$1"' _ {} \; 2>/dev/null | sort -u || true)"
if [ -n "$LEAKS" ]; then
    echo "    ! warning: still references Homebrew: $LEAKS"
fi

# 5e. Ad-hoc code-sign (arm64 refuses to launch an invalidated signature).
#     For distribution replace "-" with "Developer ID Application: <name>".
codesign --force --deep --sign - "$APP_PATH"

# 6. Smoke-test the bundled app before packaging.
echo "==> Smoke-test"
"$BIN" >/tmp/openved-smoketest.log 2>&1 &
SMOKE_PID=$!
sleep 3
if kill -0 "$SMOKE_PID" 2>/dev/null; then
    echo "    ✓ app launched and stayed up"
    kill "$SMOKE_PID" 2>/dev/null || true
    wait "$SMOKE_PID" 2>/dev/null || true
else
    wait "$SMOKE_PID" 2>/dev/null || true
    echo "    ✗ app exited immediately — see /tmp/openved-smoketest.log"
    head -5 /tmp/openved-smoketest.log
    exit 1
fi

# 7. Create the DMG
rm -f "$BUILD_DIR/$DMG_NAME"
hdiutil create -volname "OpenVED" \
    -srcfolder "$APP_PATH" \
    -ov -format UDZO \
    "$BUILD_DIR/$DMG_NAME"

echo ""
echo "==> Done: $BUILD_DIR/$DMG_NAME  ($(du -h "$BUILD_DIR/$DMG_NAME" | cut -f1))"
