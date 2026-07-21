#!/usr/bin/env bash
#
# Regenerate the bundled .vfn vector fonts from the third-party TTF/OTF sources
# using the native ved_font_converter tool.
#
# This produces the *curated* font bundle that is embedded into the app via the
# .qrc (see src/app/resources/font/). It is intentionally a small, hand-picked
# set — converting every third-party font would be ~100 MB, far too large to
# embed (and impossible for the WebAssembly build). A separate on-demand font
# loading mechanism for WASM/large sets is planned separately.
#
# The converter runs in FullCmap mode, so non-Latin scripts (Arabic, Hebrew,
# Greek, Cyrillic) are fully converted, not just Latin-1.
#
# Build the converter first:  cmake --build cmake-build-debug
# then run:                   scripts/regenerate-fonts.sh
# Override the tool with:     CONVERTER=/path/to/ved_font_converter
#
# To add a font to the bundle, add a line to the BUNDLE array:
#   "<source-path-under-third_party/fonts>|<output.vfn>|<VC:Font Name>"

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$PROJECT_ROOT/third_party/fonts"
OUT_DIR="$PROJECT_ROOT/src/app/resources/font"

# --- Curated bundle (Regular only for now) --------------------------------
# Covers: Latin (metric-compatible Arial/Times/Courier replacements), Greek +
# Cyrillic, Hebrew, and Arabic/Persian.
BUNDLE=(
    "liberation/LiberationSans-Regular.ttf|liberation_sans.vfn|VC:Liberation Sans"
    "liberation/LiberationSerif-Regular.ttf|liberation_serif.vfn|VC:Liberation Serif"
    "liberation/LiberationMono-Regular.ttf|liberation_mono.vfn|VC:Liberation Mono"
    "noto/NotoSans-Regular.ttf|noto_sans.vfn|VC:Noto Sans"
    "noto/NotoSansHebrew-Regular.ttf|noto_sans_hebrew.vfn|VC:Noto Sans Hebrew"
    "amiri/Amiri-Regular.ttf|amiri.vfn|VC:Amiri"
)
# --------------------------------------------------------------------------

# Locate the converter (env override wins).
CONVERTER="${CONVERTER:-}"
if [ -z "$CONVERTER" ]; then
    for cand in \
        "$PROJECT_ROOT/cmake-build-debug/ved_font_converter" \
        "$PROJECT_ROOT/build/release/ved_font_converter" \
        "$PROJECT_ROOT/build/ved_font_converter"; do
        if [ -x "$cand" ]; then CONVERTER="$cand"; break; fi
    done
fi
if [ -z "$CONVERTER" ] || [ ! -x "$CONVERTER" ]; then
    echo "error: ved_font_converter not found." >&2
    echo "       build it (cmake --build cmake-build-debug) or set CONVERTER=..." >&2
    exit 1
fi

mkdir -p "$OUT_DIR"

count=0
for entry in "${BUNDLE[@]}"; do
    IFS='|' read -r rel out name <<< "$entry"
    src="$SRC_DIR/$rel"
    if [ ! -f "$src" ]; then
        echo "error: source font not found: $src" >&2
        exit 1
    fi
    "$CONVERTER" --in "$src" --out "$OUT_DIR/$out" --name "$name"
    count=$((count + 1))
done

echo ""
echo "done: $count fonts -> $OUT_DIR"
du -ch "$OUT_DIR"/*.vfn | tail -1 | sed 's/total/bundle total/'
