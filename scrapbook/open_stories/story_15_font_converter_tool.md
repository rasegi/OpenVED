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
- [x] Build gruen, `ctest` 25/25 gruen, App-Smoke-Test (headless, 5s) startet
      stabil inkl. Font-Provider-Init. _(2026-07-21)_
- [ ] `TDQtSystemFontProvider` liefert identische `TDVecFont`-Ergebnisse
      (dedizierter Glyph-Kontur-Vergleichstest folgt in Paket A / Step 2).
- [ ] Native App laedt Systemfonts weiterhin korrekt (visuelle Abnahme durch User).

**Log:**
Umgesetzt am 2026-07-21 (reines Refactoring, verhaltensneutral).
- Neue Qt-freie Einheit `src/ved_fontconvert/ttf_to_vecfont.{h,cpp}` als
  Static-Lib `ved_fontconvert` (linkt `ved_core` + FreeType, kein Qt). Exportiert
  `FaceScale`, `ConvertGlyphOutline`, `ConvertTrueTypeFileToVecFont`.
- Die Konvertierungslogik (FreeType-Setup, Outline-Callbacks, `OutlineContext`,
  `faceScale`, `convertGlyphOutline`, `convertFontFileWithFreeType`) wurde
  **byte-identisch** aus `src/app/QtVecFontProviders.cpp` verschoben; nur Qt-Typen
  an der Grenze getauscht (`QString`->`std::string`).
- `TDQtSystemFontProvider` nutzt die Einheit: `LoadFont` ->
  `ConvertTrueTypeFileToVecFont` (Pfad weiterhin via `QFile::encodeName`,
  Font-Name weiterhin `fontIdForFamily` = "TT:family"); `ShapeText` ->
  `FaceScale` + `ConvertGlyphOutline`.
- Im Provider verbleiben (Qt-/HarfBuzz-gebunden): System-Font-Scan
  (`EnsureFontIndex`, nutzt weiterhin `FreeTypeLibrary`), HarfBuzz-Shaping,
  Builtin-VFN-Provider.
- CMake: neues Target `ved_fontconvert`, von `ved_qt_app` gelinkt.
- Verifikation: `cmake --build cmake-build-debug` fehlerfrei, `ctest` 25/25,
  Headless-Smoke-Test (`QT_QPA_PLATFORM=offscreen`, 5s) ohne Crash.

### Step 2: VFN-Schreibpfad kapseln

**Was:**
- `vecfont_to_vfn`: `TDVecFont` → Bytes (1024-Byte-Header + `WriteTo`-Stream),
  in Datei schreiben.
- Header-Layout an das Format von `wps_default.vfn` angleichen (erste 1024 Bytes
  Header/Reserve), damit `LoadVecFontFromMemory(headerSize=1024)` es liest.

**Tests:**
- [x] Round-Trip: `TDVecFont` → `.vfn` → `LoadVecFontFromMemory` liefert
      identischen Font (Eigenschaften + Glyph-Konturen). _(2026-07-21)_
- [x] Erzeugte `.vfn` ist strukturell kompatibel zu `wps_default.vfn` —
      sogar **byte-fuer-byte identisch** (1024-Null-Header, `'vfnt'`, Stream). _(2026-07-21)_

**Abweichung von der Story-Architektur:** Der Schreibpfad liegt in **`ved_core`**
(`vec_font.cpp`, direkt neben `LoadVecFontFromMemory`), nicht in `ved_fontconvert`
— weil er nur `TDVecFont` + `VEDBinaryWriter` nutzt (Core, kein FreeType) und so
Lesen/Schreiben symmetrisch am selben Ort liegen.

**Log:**
Umgesetzt am 2026-07-21.
- Neue Funktion `SaveVecFontToMemory(const TDVecFont&, long headerSize = 1024)
  -> std::vector<std::byte>` in `src/ved_core/fontengine/vec_font.{h,cpp}`,
  Gegenstueck zu `LoadVecFontFromMemory`. Schreibt `headerSize` Null-Bytes, den
  `'vfnt'`-FourCC (`TDVecFont::StreamFourCC()`) und den `WriteTo`-Stream.
- Byte-Layout an `wps_default.vfn` verifiziert: Header 0x00 * 1024, FourCC bei
  Offset 0x400, danach Font-Stream.
- Neues Test-Target `ved_core_vfn_save_roundtrip_tests`
  (`tests/ved_core_vfn_save_roundtrip_tests.cpp`, linkt `ved_core`): laedt
  `wps_default.vfn`, serialisiert zurueck und prueft
  (a) strukturelles Layout, (b) **byte-fuer-byte-Gleichheit** mit dem Original,
  (c) semantischen Round-Trip nach Reload, (d) Idempotenz beim erneuten Schreiben,
  (e) nicht-Default-Headergroessen (512, 0).
- Datei-Schreiben (fstream) folgt in Step 3 (CLI) als duenner Wrapper um die
  Bytes; der Core-Teil bleibt fstream-frei.
- Verifikation: `cmake --build` fehlerfrei, `ctest` **26/26** gruen.

### Step 3: CLI-Frontend `ved_font_converter`

**Was:**
- CLI-Target mit Argument-Parsing (`--in/--out/--id/--name/--face/--style`).
- Fehlerbehandlung: ungueltige Datei, nicht ladbarer Face, leere Glyph-Menge.
- Exit-Codes + verstaendliche Meldungen fuer Skript-Nutzung (Build/CI).

**Tests:**
- [x] CLI erzeugt aus echter TTF/OTF eine `.vfn`, die `LoadVecFontFromMemory`
      fehlerfrei liest — automatisiert via `ved_fontconvert_pipeline_tests`
      (TTF -> Convert -> Save -> Load) + Format-Verifikation (`'vfnt'` bei
      Offset 1024). _(2026-07-21)_
- [ ] Konvertierte `.vfn` in der App als Builtin geladen zeigt korrekten Text
      (App-Integration = Step 4 / Story 16 Step 4c).
- [x] Fehlerfaelle liefern != 0 Exit-Code + Meldung (fehlende Datei -> 1,
      Argument-Fehler -> 2). _(2026-07-21)_

**Abweichung vom Story-Entwurf:** Flags sind `--in/--out/--name/--face/--header`
(statt `--id/--style`). Der Font-Name wird als ein Parameter `--name` uebergeben
(z.B. `"VC:Liberation Sans"`); ein separates `--style` entfaellt, da der Name
alles Noetige traegt. `--header` erlaubt eine abweichende Headergroesse.

**Log:**
Umgesetzt am 2026-07-21.
- Neues **Qt-freies** CLI `src/tools/ved_font_converter/main.cpp`, Target
  `ved_font_converter` (linkt nur `ved_fontconvert` -> transitiv `ved_core` +
  FreeType), in CMake hinter `if(NOT EMSCRIPTEN)` (nicht im WASM-Build).
- Verbindet die Bausteine aus Step 1 + 2:
  `ved::fontconvert::ConvertTrueTypeFileToVecFont` -> `SaveVecFontToMemory` ->
  `std::ofstream` (der in Step 2 angekuendigte duenne fstream-Wrapper).
- Argument-Parsing manuell (keine externe Lib); Exit-Codes 0 (ok), 1
  (Konvertierungs-/IO-Fehler), 2 (Usage-Fehler); `--help`/`-h`.
- Verifikation: Build gruen; CLI real getestet auf
  `LiberationSans-Regular.ttf` (224 Zeichen) und `Amiri-Regular.ttf` (arabisch);
  erzeugte `.vfn` byte-verifiziert (1024er Null-Header + `'vfnt'`);
  Fehlerfaelle (fehlende Datei, fehlende Args) mit korrekten Exit-Codes.
- Neues Test-Target `ved_fontconvert_pipeline_tests` (linkt `ved_fontconvert`,
  konvertiert eine echte `third_party/fonts`-TTF und prueft den Save/Load-
  Round-Trip). `ctest` **27/27** gruen.

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
- [x] Fuer jeden Bundle-Font existiert eine ladbare `.vfn` in
      `src/app/resources/font/`; App baut + startet mit dem Bundle. _(2026-07-21)_
- [x] `scripts/regenerate-fonts.sh` erzeugt die `.vfn` reproduzierbar. _(2026-07-21)_
- [x] Lizenztexte liegen vollstaendig in `licenses/` und sind in
      `THIRD_PARTY_LICENSES.md` referenziert. _(2026-07-21)_

**Konverter-Erweiterung (FullCmap):** Der Konverter deckte urspruenglich nur
U+0020..U+00FF (Latin-1) ab — unzureichend fuer die Zielskripte
(Arabisch 0x600+, Hebraeisch 0x590+, Griechisch/Kyrillisch 0x370+/0x400+).
`ConvertTrueTypeFileToVecFont` bekam daher einen Parameter
`CharacterCoverage {Latin1, FullCmap}` (Default `Latin1`, damit der Qt-System-
Font-Provider **unveraendert** bleibt). `FullCmap` iteriert per
`FT_Get_First_Char`/`FT_Get_Next_Char` alle cmap-Codepoints bis U+FFFF (BMP;
`TDVecCharacter::UnicodeValue` ist 16-bit). Das CLI nutzt `FullCmap`. Belegt
durch `ved_fontconvert_pipeline_tests` (Latin-1 = 224 Zeichen; Amiri/FullCmap
enthaelt U+0627 ARABIC ALEF mit echtem Outline).

**Bundle-Strategie:** kuratierte Auswahl (Regular only), ins Binary via `.qrc`.
Alle 42 Quell-Fonts als `.vfn` waeren ~101 MB (untragbar fuers Binary, unmoeglich
fuer WASM) — daher bewusst klein gehalten. Ein separater On-demand-Lade-
Mechanismus (v.a. fuer WASM) ist spaeter geplant.

> **Ueberholt durch Step 6:** Das Bundle wird nicht mehr als `.vfn` ausgeliefert,
> sondern als **TTF** (kleiner + shapebar). Die hier beschriebene VFN-Erzeugung/
> -Registrierung ist damit historisch; aktueller Stand siehe Step 6.

**Log:**
Umgesetzt am 2026-07-21.
- `scripts/regenerate-fonts.sh` erzeugt aus einer expliziten `BUNDLE`-Liste die
  kuratierten `.vfn` (neue Fonts = eine Zeile mehr). Konverter wird
  automatisch gefunden; laeuft im `FullCmap`-Modus.
- Erzeugtes Bundle in `src/app/resources/font/` (Regular only):
  `liberation_sans` (2327 Zeichen), `liberation_serif` (2321),
  `liberation_mono` (2305), `noto_sans` (2838, Greek/Cyrillic),
  `noto_sans_hebrew` (143), `amiri` (1556, Arabisch) — zusammen ~13 MB;
  `wps_default.vfn` bleibt.
- Abdeckung: Latin (Arial/Times/Courier-Ersatz), Griechisch, Kyrillisch,
  Hebraeisch, Arabisch/Persisch.
- App gebaut: Binary 3,1 MB -> **4,2 MB** (Qt-`rcc` komprimiert die Vektordaten
  ~13 MB -> ~1 MB). Headless-Smoke-Test 5 s stabil.
- Auto-Registrierung ergaenzt (2026-07-21): alle gebuendelten `.vfn` werden beim
  Font-Setup **automatisch** registriert (kein Hardcoding). Neue Fonts =
  Script-Zeile + `.vfn` in `resources/font/`, sonst nichts. Legacy `wps_default`
  (ohne eingebetteten Namen) bleibt der explizite Default; die uebrigen `.vfn`
  liefern ihre Font-ID ueber `PeekVfnFontName` (liest nur Header + Name).

### Step 5: Menue-Switch "Convert System Fonts" (persistiert, WASM-aware)

**Was:**
- Checkbarer Menue-Eintrag im Format-Menue: "Convert System Fonts" (Haekchen).
- Steuert, ob installierte TrueType/OpenType-Systemfonts gescannt, konvertiert
  und im Provider bereitgestellt werden — und damit in Text-Operationen /
  Font-Auswahl erscheinen. Aus = nur gebuendelte VFN-Fonts.
- Zustand in QSettings persistiert (`fonts/convertSystemFonts`).
- In WASM **deaktiviert und ausgegraut** (keine Systemfonts im Browser).

**Umsetzung:**
- `MainWindow::systemFontsEnabled()` = Action-Checked (immer `false` auf WASM
  via `#if defined(Q_OS_WASM)`).
- `rebuildFontProviders()` (aus `loadDefaultVecFont` extrahiert) registriert den
  `TDQtSystemFontProvider` nur bei aktivem Switch; sonst No-op-Shaper.
- `onToggleSystemFonts(bool)` persistiert + baut Provider neu + Combobox-Refresh.
- `populateTextFontCombo(bool force)` erlaubt erzwungenes Neu-Befuellen.
- Menue-Action: `connect` nach initialem `setChecked`, damit Start kein
  `toggled` ausloest.

**Verhaltensaenderung:** Systemfonts sind jetzt **opt-in** (Default aus). Vorher
wurden sie immer geladen; Grund fuer den Switch ist, dass Scan/Convert teuer ist.

**Tests:**
- [x] Build gruen, `ctest` 27/27, App startet stabil (Default: Systemfonts aus,
      nur Bundle-Fonts). _(2026-07-21)_
- [ ] Toggle an -> Systemfonts erscheinen in der Font-Auswahl; aus -> verschwinden
      (visuelle Abnahme durch User).
- [ ] Zustand ueberlebt Neustart (QSettings).

**Log:**
Umgesetzt am 2026-07-21. Geaendert: `MainWindow.{h,cpp}`,
`MainWindowTextDock.cpp`. WASM-Verhalten zusaetzlich in `story_16` ergaenzt.

### Step 6: Bugfix — gebuendelte Fonts als TTF + Shaping (Arabisch/RTL)

**Bug:** Persischer/arabischer Text mit dem gebuendelten `Amiri`-Font wurde
falsch gerendert — isolierte Buchstabenformen, keine Verbindungen/Ligaturen und
falsche Richtung (LTR statt RTL). Beleg: `Error-Images/Fehler_bei_Amiri_Font.jpg`.

**Ursache (zweifach):**
1. Der HarfBuzz-Shaper lief nur fuer System-Fonts, nicht fuer die gebuendelten
   `.vfn`-Fonts.
2. **Grundsaetzlich:** Das VFN-Format speichert nur Unicode→Glyph-Outline, aber
   **keine OpenType-Shaping-Tabellen** (`GSUB`/`GPOS`). Arabisches/persisches
   Shaping ist mit `.vfn` daher prinzipiell unmoeglich — HarfBuzz braucht die
   originale TTF.

**Loesung:** Gebuendelte Fonts werden als **originale TTF** ausgeliefert (nicht
mehr VFN) und ueber HarfBuzz geshapt. Die TTF sind zudem kleiner als die VFN
(z.B. Amiri 588 KB vs. 3,5 MB).

**Umsetzung (Teile 1–5):**
- Konverter: `ConvertTrueTypeMemoryToVecFont` (TTF aus Qt-Ressourcen via
  `FT_New_Memory_Face`), geteilter Kern `convertFaceToVecFont`. Der VFN-Konverter
  + CLI bleiben erhalten (fuer `wps_default`/Spezialfaelle), werden aber fuer das
  Bundle nicht mehr genutzt.
- `scripts/regenerate-fonts.sh` kopiert jetzt die kuratierten Bundle-**TTF** nach
  `src/app/resources/font/` und entfernt die alten Bundle-`.vfn`
  (`wps_default.vfn` bleibt).
- `TDQtSystemFontProvider`: indexiert **immer** die gebuendelten TTF (Ressourcen,
  Memory-Face) und **nur bei aktivem Switch** die installierten System-Fonts;
  `LoadFont`/`ShapeText` bedienen beide Quellen (Memory- vs. Pfad-Face). Der
  Provider wird immer registriert; der Switch steuert nur den System-Anteil.
- `.qrc` bettet `*.ttf` mit ein.

**Namenskonvention (neu):**
- Gebuendelte Ressourcen-Fonts (TTF **und** `wps_default.vfn`): Praefix **`Ved:`**
  (z.B. `Ved:Amiri`, `Ved:WPS Default`).
- Installierte System-Fonts: Praefix **`Sys:`** (loest das alte `TT:` ab).
- Die Font-Auswahl zeigt die volle ID mit Praefix.
- **Kein Rueckwaerts-Kompat:** alte Dokumente mit `VC:`/`TT:`-Referenzen fallen
  auf den Default zurueck (Grundsatz „keine Rueckwaertskompatibilitaet").

**Tests:**
- [x] Build gruen, `ctest` 27/27, App startet stabil (Bundle-TTF immer im Index,
      Shaper aktiv). _(2026-07-21)_
- [ ] `Ved:Amiri` rendert persisch/arabisch **verbunden und RTL** (visuelle
      Abnahme durch User).
- [ ] Switch an -> zusaetzlich `Sys:`-Fonts, ebenfalls geshapt.

**Log:**
Umgesetzt am 2026-07-21. Geaendert: `ttf_to_vecfont.{h,cpp}`,
`QtVecFontProviders.{h,cpp}`, `MainWindowTextDock.cpp`,
`scripts/regenerate-fonts.sh`, `CMakeLists.txt` (qrc `*.ttf`). Bundle-Fonts von
`.vfn` auf `.ttf` umgestellt (alte `.vfn` entfernt, TTF hinzugefuegt).

## Akzeptanzkriterien

- Natives CLI `ved_font_converter` erzeugt aus TTF/OTF eine `.vfn`, die
  `LoadVecFontFromMemory` liest und die App als Font darstellt.
- Die Konvertierungslogik ist aus `MainWindow`/App entkoppelt und wird von
  Provider **und** CLI geteilt; `ved_core` bleibt Qt-frei.
- Das Tool ist hinter `if(NOT EMSCRIPTEN)` gegated (nicht im WASM-Build).
- Ein Basis-Set frei lizenzierter Fonts (Liberation/Noto/Amiri) ist gebuendelt
  und lizenzdokumentiert. **Ab Step 6 als TTF** (statt VFN), damit HarfBuzz
  komplexe Schriften (Arabisch/Persisch, Hebraeisch) korrekt shapt.
- Gebuendelte Fonts (`Ved:`) sind immer verfuegbar; System-Fonts (`Sys:`) nur bei
  aktivem "Convert System Fonts"-Switch — beide werden geshapt.

## Reihenfolge

1. Step 1 — Konvertierungslogik entkoppeln (Provider unveraendert).
2. Step 2 — VFN-Schreibpfad kapseln (Round-Trip gruen).
3. Step 3 — CLI-Frontend.
4. Step 4 — Bundle-Fonts konvertieren + Lizenzen + Auto-Registrierung.
5. Step 5 — Menue-Switch "Convert System Fonts" (persistiert, WASM-aware).
6. Step 6 — Bugfix: gebuendelte Fonts als TTF + HarfBuzz-Shaping (Arabisch/RTL);
   Praefixe `Ved:`/`Sys:`.

## Bezug zu anderen Stories

- **`story_16_webassembly.md` ist von dieser Story abhaengig** — das
  WASM-Font-Bundle (Step 4b dort) nutzt die hier erzeugten `.vfn`-Dateien und
  das CLI. Story 15 wird vor dem Font-Teil von Story 16 umgesetzt.
- `story_font_provider_truetype_converter_plugin.md` — dieselbe
  Konvertierungs-Grundlage; das spaetere Laufzeit-Plugin und dieses
  Build-Zeit-CLI teilen sich die entkoppelte `ttf_to_vecfont`-Einheit.
- `story_document_font_packaging.md` — kann konvertierte `.vfn` fuer
  Dokument-Font-Embedding wiederverwenden.
