#!/usr/bin/env bash
#
# Copy the curated font bundle into the app resources.
#
# The bundled fonts are shipped as original TTF (not converted VFN) so the app
# can shape them with HarfBuzz (RTL, contextual forms, ligatures) — essential
# for Arabic/Persian, Hebrew, etc. Only wps_default stays as a legacy .vfn.
#
# The TTF are embedded into the app via the .qrc (GLOB *.ttf in CMakeLists.txt)
# and loaded by the resource font provider at runtime.
#
# To add a font to the bundle, add a line to the BUNDLE array:
#   "<source-path-under-third_party/fonts>|<output-name.ttf>"

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$PROJECT_ROOT/third_party/fonts"
OUT_DIR="$PROJECT_ROOT/src/app/resources/font"

# --- Curated bundle (Regular only for now) --------------------------------
# Covers: Latin (metric-compatible Arial/Times/Courier replacements), Greek +
# Cyrillic, Hebrew, and Arabic/Persian.
BUNDLE=(
    "liberation/LiberationSans-Regular.ttf|liberation_sans.ttf"
    "liberation/LiberationSerif-Regular.ttf|liberation_serif.ttf"
    "liberation/LiberationMono-Regular.ttf|liberation_mono.ttf"
    "noto/NotoSans-Regular.ttf|noto_sans.ttf"
    "noto/NotoSansHebrew-Regular.ttf|noto_sans_hebrew.ttf"
    "amiri/Amiri-Regular.ttf|amiri.ttf"
)
# --------------------------------------------------------------------------

mkdir -p "$OUT_DIR"

# Drop any previously generated bundle .vfn (keep only the legacy wps_default).
shopt -s nullglob
for vfn in "$OUT_DIR"/*.vfn; do
    if [ "$(basename "$vfn")" != "wps_default.vfn" ]; then
        rm -f "$vfn"
        echo "removed old vfn: $(basename "$vfn")"
    fi
done

count=0
for entry in "${BUNDLE[@]}"; do
    IFS='|' read -r rel out <<< "$entry"
    src="$SRC_DIR/$rel"
    if [ ! -f "$src" ]; then
        echo "error: source font not found: $src" >&2
        exit 1
    fi
    cp "$src" "$OUT_DIR/$out"
    echo "copied: $rel -> $out"
    count=$((count + 1))
done

echo ""
echo "done: $count TTF fonts -> $OUT_DIR"
du -ch "$OUT_DIR"/*.ttf "$OUT_DIR"/*.vfn 2>/dev/null | tail -1 | sed 's/total/bundle total/'
