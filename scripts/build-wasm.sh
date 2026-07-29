#!/usr/bin/env bash
#
# Build OpenVED for the browser (WebAssembly) and collect a static web bundle.
#
#   scripts/build-wasm.sh
#
# Output: build/wasm/dist/ with index.html + OpenVED.{js,wasm} + qtloader.js,
# ready to host on any static server (e.g. GitHub Pages).
#
# Toolchain (Qt 6.11 ↔ Emscripten 4.0.7, see story_16). Paths are taken from
# environment variables so the same script works locally and in CI:
#   EMSDK     path to an emsdk checkout (its emsdk_env.sh is sourced if present)
#   QT_WASM   Qt-for-WebAssembly kit   (default: ~/Qt/6.11.1/wasm_singlethread)
#   QT_HOST   host Qt for moc/rcc      (default: ~/Qt/6.11.1/macos)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build/wasm"
DIST_DIR="${BUILD_DIR}/dist"

QT_WASM="${QT_WASM:-$HOME/Qt/6.11.1/wasm_singlethread}"
QT_HOST="${QT_HOST:-$HOME/Qt/6.11.1/macos}"

# Activate Emscripten (the Qt wasm toolchain wraps it).
if [ -n "${EMSDK:-}" ] && [ -f "$EMSDK/emsdk_env.sh" ]; then
    # shellcheck disable=SC1091
    source "$EMSDK/emsdk_env.sh" >/dev/null 2>&1
fi
command -v emcc >/dev/null 2>&1 || {
    echo "error: emcc not on PATH — set EMSDK or 'source <emsdk>/emsdk_env.sh' first" >&2
    exit 1
}

echo "==> OpenVED — WebAssembly build"
echo "    Qt wasm : $QT_WASM"
echo "    Qt host : $QT_HOST"
echo "    emcc    : $(emcc --version | head -1)"

# 1. Configure (Qt wasm toolchain pulls in Emscripten; FreeType/HarfBuzz via FetchContent)
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$QT_WASM/lib/cmake/Qt6/qt.toolchain.cmake" \
    -DQT_HOST_PATH="$QT_HOST" \
    -DCMAKE_BUILD_TYPE=Release

# 2. Build the app (Emscripten emits OpenVED.html/.js/.wasm + qtloader.js)
cmake --build "$BUILD_DIR" --target ved_qt_app

# 3. Collect the static bundle
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"
cp "$BUILD_DIR"/OpenVED.js "$BUILD_DIR"/OpenVED.wasm "$DIST_DIR/"
cp "$BUILD_DIR"/qtloader.js "$DIST_DIR/" 2>/dev/null || true
cp "$BUILD_DIR"/qtlogo.svg   "$DIST_DIR/" 2>/dev/null || true
# GitHub Pages serves index.html at the site root; the generated HTML already
# references OpenVED.js, so a copy under index.html is all that's needed.
cp "$BUILD_DIR"/OpenVED.html "$DIST_DIR/index.html"

echo ""
echo "==> Bundle ready: $DIST_DIR"
ls -lh "$DIST_DIR" | awk 'NR>1 {print "    " $9 "  " $5}'
