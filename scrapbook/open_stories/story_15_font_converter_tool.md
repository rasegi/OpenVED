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

Freie, redistribuierbare Fonts — metrik-kompatibler Ersatz fuer die
MS-Kernschriften plus breite Unicode-/Arabic-Abdeckung:

| Zweck | Font | Lizenz | Ersetzt / Zweck |
|---|---|---|---|
| Sans | Liberation Sans (Regular/Bold) | OFL 1.1 | Arial |
| Serif | Liberation Serif (Regular) | OFL 1.1 | Times New Roman |
| Mono | Liberation Mono (Regular) | OFL 1.1 | Courier New |
| Unicode-Breite | DejaVu Sans/Serif/Mono | Bitstream Vera / public | breite Latin/Symbol-Abdeckung |
| Arabic/Naskh | Amiri (Regular/Bold/Italic) | OFL 1.1 | arabischer/persischer RTL-Text |
| Latin/Greek/Cyrillic | Noto Sans + Noto Serif (R/B/I) | OFL 1.1 | latein, griechisch, slavisch (kyrillisch) |
| Hebrew | Noto Sans/Serif Hebrew (R/B) | OFL 1.1 | hebräischer Text |
| Arabic/Persian | Noto Sans Arabic + Noto Naskh Arabic (R/B) | OFL 1.1 | arabisch/persisch (RTL), Noto-Alternative zu Amiri |

Hinweis: **Arial und Courier New selbst sind proprietaer** (Monotype) und
duerfen nicht gebuendelt/konvertiert werden — daher die Liberation-Familie.
Hinweis Amiri: `AmiriQuran*`-Varianten (u.a. `AmiriQuranColored` als COLR/CPAL-
Farbfont) sind fuer die VFN-Outline-Konvertierung ungeeignet und werden **nicht**
konvertiert; sie liegen nur zur Vollstaendigkeit in `third_party/fonts/amiri/`.

**Font-Bezug (Ist-Zustand, bereits im Repo):** gnu.org lieferte nur die
LGPL/GPL-Rechtstexte, **nicht** die Fonts.

| Font | Quelle | Version | Bemerkung |
|---|---|---|---|
| DejaVu (Sans/Serif/Mono) | github.com/dejavu-fonts/dejavu-fonts (Release-Archiv) | 2.37 | offizielle fertige TTF |
| Liberation (Sans/Serif/Mono) | lokale LibreOffice-Installation (26.2.4.2) | 2.1.5 | siehe unten |
| Amiri (Regular/Bold/Italic + Quran) | github.com/aliftype/amiri (main, `fonts/`) | ~1.003 | offizielle fertige TTF, Arabic |
| Noto (Sans/Serif, Hebrew, Arabic, Naskh) | github.com/googlefonts/noto-fonts (`hinted/ttf/`) | main | offizielle fertige TTF; Latin/Greek/Cyrillic/Hebrew/Arabic |

Hinweis Liberation: Es gibt **keine offiziellen fertigen Liberation-TTF zum
Download** — die GitHub-Releases enthalten nur `.sfd`-Quellen, und `fontforge`
zum Selbstbauen war nicht verfuegbar. Daher wurden die TTF aus dem
LibreOffice-Bundle (`/Applications/LibreOffice.app/Contents/Resources/fonts/
truetype/`) kopiert. Die dort mitgelieferte Version ist laut Font-Metadaten
**Liberation 2.1.5 — identisch zur offiziellen Upstream-Version** und
unmodifiziert, die Lizenzlage (OFL 1.1) ist damit sauber.

**Ablage im Repo (beide Artefakte committet):**
```
third_party/fonts/liberation/   Liberation*.ttf   (v2.1.5, aus LibreOffice, 16 Dateien)
third_party/fonts/dejavu/        DejaVu*.ttf       (v2.37, offiziell, 6 Dateien)
third_party/fonts/amiri/         Amiri*.ttf        (~v1.003, offiziell, 6 Dateien)
third_party/fonts/noto/          Noto*.ttf         (main, offiziell, 16 Dateien)
src/app/resources/font/          *.vfn             (konvertiert, Qt-Ressource)
```
Grund fuers Committen der `.vfn`: `ved_font_converter` ist ein **natives** Tool
und laeuft beim WASM-Cross-Build nicht — die `.vfn` koennen dort nicht
build-zeitlich erzeugt werden und muessen fertig im Repo liegen. Die TTF-Quellen
werden mitcommittet, damit die `.vfn` jederzeit reproduzierbar neu erzeugbar sind.

**Was:**
- Quell-TTF liegen bereits in `third_party/fonts/` (Liberation 2.1.5, DejaVu
  2.37) — umgesetzt am 2026-07-21, committet.
- Mit `ved_font_converter` einmalig nach `.vfn` konvertieren, Ergebnis nach
  `src/app/resources/font/` (committet), `.qrc` ergaenzen.
- `scripts/regenerate-fonts.sh` mit den konkreten Konverter-Aufrufen ablegen,
  damit die Erzeugung dokumentiert und wiederholbar ist.
- Lizenztexte in der zentralen `licenses/` fuehren (`THIRD_PARTY_LICENSES.md`);
  OFL-1.1 und DejaVu-Lizenz sind bereits eingebunden.

**Tests:**
- [x] Quell-TTFs liegen versioniert in `third_party/fonts/`. _(2026-07-21)_
- [ ] Fuer jeden Bundle-Font existiert eine ladbare `.vfn` in `src/app/resources/font/`.
- [ ] `scripts/regenerate-fonts.sh` erzeugt die `.vfn` reproduzierbar aus den Quellen.
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
