#!/usr/bin/env bash
#
# Regenerate all bundled .vfn vector fonts from the third-party TTF/OTF sources
# using the native ved_font_converter tool.
#
# Sources: third_party/fonts/<project>/*.ttf
# Output:  src/app/resources/font/*.vfn   (embedded into the app via .qrc)
#
# Build the converter first:
#   cmake --build cmake-build-debug
# then run:
#   scripts/regenerate-fonts.sh
#
# Override the converter path with CONVERTER=/path/to/ved_font_converter.
#
# NOTE: the current converter only emits characters U+0020..U+00FF (Latin-1).
# Non-Latin sources (Amiri, Noto Arabic/Hebrew, Greek/Cyrillic) therefore only
# yield their Latin glyphs until the converter's character range is extended.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$PROJECT_ROOT/third_party/fonts"
OUT_DIR="$PROJECT_ROOT/src/app/resources/font"

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

# Source glyphs unsuitable for monochrome outline conversion (color / special).
SKIP_REGEX='AmiriQuran'

# Derive a stable, readable font name from a file basename:
#   "LiberationSans-Regular" -> "VC:Liberation Sans Regular"
font_name() {
    local name
    name="$(printf '%s' "$1" \
        | sed -E 's/([a-z0-9])([A-Z])/\1 \2/g' \
        | tr '-' ' ' \
        | tr -s ' ')"
    printf 'VC:%s' "$name"
}

mkdir -p "$OUT_DIR"

converted=0
skipped=0
shopt -s nullglob
for ttf in "$SRC_DIR"/*/*.ttf; do
    base="$(basename "$ttf" .ttf)"
    if printf '%s' "$base" | grep -qE "$SKIP_REGEX"; then
        printf 'skip:  %s (unsuitable for outline conversion)\n' "$base"
        skipped=$((skipped + 1))
        continue
    fi
    out="$OUT_DIR/$(printf '%s' "$base" | tr 'A-Z-' 'a-z_').vfn"
    name="$(font_name "$base")"
    "$CONVERTER" --in "$ttf" --out "$out" --name "$name"
    converted=$((converted + 1))
done

printf '\ndone: %d converted, %d skipped -> %s\n' "$converted" "$skipped" "$OUT_DIR"
