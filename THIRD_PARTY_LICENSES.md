# Third-Party Licenses

OpenVED itself is released under the MIT License (see `LICENSE`).

OpenVED bundles and/or links against the third-party components listed below.
Their full license texts are in the `licenses/` directory and are shipped with
every distributed build (native DMG/MSI and the WebAssembly web bundle).

---

## Qt 6 (Widgets, Gui, PrintSupport)

- **License:** GNU LGPL version 3 — see `licenses/Qt-LGPL-3.0.txt`
  (the LGPLv3 incorporates the GPLv3, see `licenses/Qt-GPL-3.0.txt`).
- **Website / source:** https://www.qt.io — Qt source code is available from
  https://download.qt.io.
- **Linking:**
  - **Native (macOS/Windows):** Qt is linked **dynamically** (shared
    frameworks/DLLs, deployed via macdeployqt/windeployqt). This satisfies the
    LGPLv3 requirement that the end user can replace the Qt libraries.
  - **WebAssembly:** Qt for WebAssembly is linked **statically**. Under LGPLv3,
    static linking requires that the recipient be able to relink OpenVED against
    a modified Qt. This is covered because OpenVED's complete source and build
    instructions are publicly available (MIT, on GitHub), allowing anyone to
    rebuild the WASM bundle against their own Qt. This must remain true for every
    published WASM release.

## FreeType 2

- **License:** The FreeType License (FTL, BSD-style with credit clause) —
  see `licenses/FreeType-FTL.txt`. (FreeType is dual-licensed FTL / GPLv2;
  OpenVED uses it under the FTL.)
- **Website / source:** https://www.freetype.org
- **Linking:** static.
- **Required credit** (FTL clause 3):

  > Portions of this software are copyright © The FreeType Project
  > (www.freetype.org). All rights reserved.

## HarfBuzz

- **License:** "Old MIT" license — see `licenses/HarfBuzz-COPYING.txt`.
- **Website / source:** https://harfbuzz.github.io
- **Linking:** static.

## Bundled fonts — Liberation family

Converted to VED's `.vfn` format and embedded as application resources
(see `story_15_font_converter_tool.md`). The Liberation fonts are metric-
compatible, freely redistributable replacements for the proprietary Microsoft
core fonts (Arial → Liberation Sans, Times New Roman → Liberation Serif,
Courier New → Liberation Mono).

- **License:** SIL Open Font License 1.1 — see `licenses/Liberation-OFL-1.1.txt`.
- **Version:** 2.1.5 (`third_party/fonts/liberation/`).
- **Source:** Upstream project is
  https://github.com/liberationfonts/liberation-fonts, which publishes only
  `.sfd` sources (no prebuilt TTF downloads). The bundled TTF were copied from a
  local LibreOffice installation, which ships the **unmodified** Liberation
  2.1.5 fonts; their metadata confirms they match the upstream release. The OFL
  permits this redistribution.
- **Reserved Font Names:** as stated in the OFL notice.

## Bundled fonts — DejaVu (optional, wider Unicode coverage)

- **License:** Bitstream Vera License + public-domain additions —
  see `licenses/DejaVu-LICENSE.txt`.
- **Version:** 2.37 (`third_party/fonts/dejavu/`).
- **Source:** https://dejavu-fonts.github.io / github.com/dejavu-fonts/dejavu-fonts.

## Bundled fonts — Amiri (Arabic / Naskh)

Arabic/Persian RTL text coverage.

- **License:** SIL Open Font License 1.1 — see `licenses/Amiri-OFL-1.1.txt`.
- **Version:** ~1.003 (`third_party/fonts/amiri/`).
- **Source:** https://github.com/aliftype/amiri (main branch, `fonts/`).
- **Note:** The `AmiriQuran*` variants (incl. `AmiriQuranColored`, a COLR/CPAL
  color font) are unsuitable for monochrome outline conversion to `.vfn` and are
  kept for completeness only — they are not converted.

---

## Notes for maintainers

- Any new bundled or linked component must add its full license text under
  `licenses/` and an entry here **before** it ships in a release.
- `licenses/` is packaged into all three distribution paths (DMG, MSI, WASM).
- The proprietary fonts Arial, Times New Roman and Courier New are **not**
  bundled — only their free metric-compatible Liberation equivalents.
