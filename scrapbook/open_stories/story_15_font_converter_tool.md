# Story 15: TTF/OTF → VFN Font-Converter-Tool

Datum: 2026-07-20
Status: offen

## Kontext

OpenVED-Text ist kein Qt-/System-Text, sondern interne VED-Vektorfonts
(`TDVecFont` → `TDVecCharacter` → `TDVecGlyph` aus `TDVecPolyCurve`-Konturen).
Das eigene Fontformat ist `.vfn` (1024-Byte-Header + serialisierter
`TDVecFont`-Stream mit Markern `vfnt`/`vchr`/`vgly`/`plyc`). Der Default-Font
`src/app/resources/font/wps_default.vfn` ist bereits so eingebunden.

Die Bausteine fuer eine Konvertierung existieren schon, sind aber im Qt-App-Layer
verstreut und nur zur Laufzeit nutzbar:

- **TTF/OTF → `TDVecFont`:** `convertFontFileWithFreeType()` (FreeType +
  HarfBuzz, Glyph-Outlines → `TDVecPolyCurve`) in `src/app/QtVecFontProviders.cpp`.
- **`TDVecFont` → `.vfn`-Stream:** `TDVecFont::WriteTo(VEDBinaryWriter&)`
  (`src/ved_core/fontengine/vec_font.cpp:574`), analog fuer Character/Glyph.
- **`.vfn` → `TDVecFont`:** `LoadVecFontFromMemory(data, size, headerSize=1024)`.

Bisher gibt es **kein Werkzeug**, das aus einer TTF/OTF eine persistente
`.vfn`-Datei erzeugt. Diese Faehigkeit wird gebraucht, um mitgelieferte
Font-Bundles (z.B. fuer den WebAssembly-Build) zur Build-Zeit zu erzeugen, sodass
der Browser nur fertige `.vfn` liest und kein TTF/FreeType-Handling zur Laufzeit
braucht.

## Ziel

Ein natives CLI-Tool `ved_font_converter`, das TTF/OTF-Fonts in das
`.vfn`-Format konvertiert — als Build-/Entwickler-Werkzeug, nicht Teil der
laufenden App.

## Nicht-Ziele

- Keine Qt-Abhaengigkeit im `ved_core` (harter Grundsatz bleibt).
- Kein Font-Editor, kein Font-Subsetting (kann spaeter folgen).
- Keine Laufzeit-Konvertierung in der App aendern — der bestehende
  `TDQtSystemFontProvider` bleibt wie er ist.
- Keine Lizenzumgehung: nur frei redistribuierbare/einbettbare Fonts konvertieren.
- Kein Teil des WebAssembly-Builds (das Tool laeuft nativ zur Build-Zeit).

## Architektur

Die heute in `QtVecFontProviders.cpp` verwobene Konvertierungslogik
(FreeType-Face-Handling, HarfBuzz-Shaping, `convertGlyphOutline`,
`convertFontFileWithFreeType`) wird in eine **wiederverwendbare Einheit**
gezogen, die sowohl der bestehende Provider als auch das neue CLI nutzen. Diese
Einheit darf Qt/FreeType/HarfBuzz nutzen, gehoert aber **nicht** in `ved_core`.

Moegliche Ablage:

```
src/ved_qt/fontconvert/           # oder src/tools/fontconvert/
  ttf_to_vecfont.h / .cpp         # TTF/OTF -> TDVecFont (FreeType/HarfBuzz)
  vecfont_to_vfn.h / .cpp         # TDVecFont -> .vfn (1024-Header + WriteTo)
src/tools/ved_font_converter/
  main.cpp                        # CLI-Frontend
```

CMake: neues natives Target `ved_font_converter` (`add_executable`), **hinter
`if(NOT EMSCRIPTEN)` gegated**, linkt `ved_core` + FreeType/HarfBuzz (+ ggf. Qt
Gui, falls `QRawFont`/`QPainterPath` als Konvertierungsbasis genutzt wird).

CLI-Skizze:

```
ved_font_converter --in <font.ttf> --out <font.vfn> \
                   --id "VC:Liberation Sans" --name "Liberation Sans" \
                   [--face <index>] [--style regular|bold|italic]
```

## Steps

### Step 1: Konvertierungslogik entkoppeln

**Was:**
- `convertFontFileWithFreeType()` / `convertGlyphOutline()` und das
  FreeType/HarfBuzz-Setup aus `src/app/QtVecFontProviders.cpp` in eine eigene
  Einheit (`ttf_to_vecfont`) extrahieren.
- `TDQtSystemFontProvider` auf diese Einheit umstellen (Verhalten unveraendert).

**Tests:**
- [ ] `TDQtSystemFontProvider` liefert nach dem Refactoring identische
      `TDVecFont`-Ergebnisse wie zuvor (Glyph-Kontur-Vergleich mit Toleranz).
- [ ] Native App laedt Systemfonts weiterhin korrekt.

**Log:** _(nach Umsetzung ausfuellen)_

### Step 2: VFN-Schreibpfad kapseln

**Was:**
- `vecfont_to_vfn`: `TDVecFont` → Bytes (1024-Byte-Header + `WriteTo`-Stream),
  in Datei schreiben.
- Header-Layout an das Format von `wps_default.vfn` angleichen (erste 1024 Bytes
  Header/Reserve), damit `LoadVecFontFromMemory(headerSize=1024)` es liest.

**Tests:**
- [ ] Round-Trip: `TDVecFont` → `.vfn` → `LoadVecFontFromMemory` liefert
      geometrisch identischen Font.
- [ ] Eine erzeugte `.vfn` ist strukturell kompatibel zu `wps_default.vfn`
      (Marker-Reihenfolge, Header-Groesse).

**Log:** _(nach Umsetzung ausfuellen)_

### Step 3: CLI-Frontend `ved_font_converter`

**Was:**
- CLI-Target mit Argument-Parsing (`--in/--out/--id/--name/--face/--style`).
- Fehlerbehandlung: ungueltige Datei, nicht ladbarer Face, leere Glyph-Menge.
- Exit-Codes + verstaendliche Meldungen fuer Skript-Nutzung (Build/CI).

**Tests:**
- [ ] `ved_font_converter --in dejavu.ttf --out dejavu.vfn ...` erzeugt eine
      Datei, die `LoadVecFontFromMemory` fehlerfrei liest.
- [ ] Konvertierte `.vfn` in der App als Builtin geladen zeigt korrekten Text.
- [ ] Fehlerfaelle liefern != 0 Exit-Code + Meldung.

**Log:** _(nach Umsetzung ausfuellen)_

### Step 4: Font-Auswahl fuer das Basis-Bundle

Freie, redistribuierbare, metrik-kompatible Fonts (Ersatz fuer die MS-Kernschriften):

| Zweck | Font | Lizenz | Ersetzt |
|---|---|---|---|
| Sans | Liberation Sans (Regular/Bold) | OFL/Apache | Arial |
| Serif | Liberation Serif (Regular) | OFL/Apache | Times New Roman |
| Mono | Liberation Mono (Regular) | OFL/Apache | Courier New |
| Unicode-Breite (optional) | DejaVu Sans | Bitstream Vera / public | — |

Hinweis: **Arial und Courier New selbst sind proprietaer** (Monotype) und
duerfen nicht gebuendelt/konvertiert werden — daher die Liberation-Familie.

**Font-Bezug:** Die Liberation-/DejaVu-TTFs sind auf macOS **nicht systemweit
installiert** — sie liegen nur app-intern (z.B. im LibreOffice-Bundle unter
`/Applications/LibreOffice.app/Contents/Resources/fonts/truetype/`). Deshalb
werden sie **offiziell frisch bezogen** (definierte Version + originaler
Lizenztext), nicht aus fremden App-Bundles kopiert:
- Liberation: https://github.com/liberationfonts/liberation-fonts
- DejaVu: https://dejavu-fonts.github.io

**Was:**
- Fonts von der offiziellen Quelle beziehen, mit dem CLI nach `.vfn` konvertieren.
- Lizenztexte gehoeren in die zentrale `licenses/` (Repo-Root) und werden in
  `THIRD_PARTY_LICENSES.md` gefuehrt — nicht in einer separaten Font-Datei.
  Die OFL-1.1 (`licenses/Liberation-OFL-1.1.txt`) ist bereits eingebunden.

**Tests:**
- [ ] Fuer jeden Bundle-Font existiert eine ladbare `.vfn`.
- [ ] Lizenztexte liegen vollstaendig in `licenses/` und sind in
      `THIRD_PARTY_LICENSES.md` referenziert.

**Log:** _(nach Umsetzung ausfuellen)_

## Akzeptanzkriterien

- Natives CLI `ved_font_converter` erzeugt aus TTF/OTF eine `.vfn`, die
  `LoadVecFontFromMemory` liest und die App als Font darstellt.
- Die Konvertierungslogik ist aus `MainWindow`/App entkoppelt und wird von
  Provider **und** CLI geteilt; `ved_core` bleibt Qt-frei.
- Das Tool ist hinter `if(NOT EMSCRIPTEN)` gegated (nicht im WASM-Build).
- Ein Basis-Set frei lizenzierter `.vfn`-Fonts (Liberation-Familie) liegt
  konvertiert + lizenzdokumentiert vor.

## Reihenfolge

1. Step 1 — Konvertierungslogik entkoppeln (Provider unveraendert).
2. Step 2 — VFN-Schreibpfad kapseln (Round-Trip gruen).
3. Step 3 — CLI-Frontend.
4. Step 4 — Bundle-Fonts konvertieren + Lizenzen.

## Bezug zu anderen Stories

- **`story_16_webassembly.md` ist von dieser Story abhaengig** — das
  WASM-Font-Bundle (Step 4b dort) nutzt die hier erzeugten `.vfn`-Dateien und
  das CLI. Story 15 wird vor dem Font-Teil von Story 16 umgesetzt.
- `story_font_provider_truetype_converter_plugin.md` — dieselbe
  Konvertierungs-Grundlage; das spaetere Laufzeit-Plugin und dieses
  Build-Zeit-CLI teilen sich die entkoppelte `ttf_to_vecfont`-Einheit.
- `story_document_font_packaging.md` — kann konvertierte `.vfn` fuer
  Dokument-Font-Embedding wiederverwenden.
