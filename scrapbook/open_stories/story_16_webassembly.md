# Story 16: OpenVED als WebAssembly-App im Browser

Datum: 2026-07-20
Status: offen

**Abhaengigkeit:** Diese Story setzt `story_15_font_converter_tool.md` voraus —
das dort definierte `ved_font_converter`-CLI erzeugt die `.vfn`-Fonts fuer das
WASM-Basis-Bundle (Step 4). Story 15 wird vor dem Font-Teil dieser Story
umgesetzt.

## Kontext

OpenVED soll zusaetzlich zu den nativen Desktop-Builds (DMG/MSI, siehe
`plan_installer_build_github_distribution.md`) als **WebAssembly-Anwendung im
Browser** lauffaehig werden — ueber Qt for WebAssembly (Emscripten-Toolchain).

Die Ausgangslage ist guenstig:

- `ved_core/` (~23k LOC) ist **vollstaendig Qt-frei** (0 Qt-Includes) und nutzt
  nur STL + FreeType/HarfBuzz. Kompiliert praktisch unveraendert mit Emscripten.
- Rendering laeuft ueber die Abstraktion `TDGraphicEngine`; QPainter existiert
  nur in `ved_qt/gengine/`. Qt for WebAssembly unterstuetzt QPainter + QWidgets
  voll (Software-Rendering auf Canvas/WebGL).
- Keine Threads (`QThread`/`std::thread`/`QtConcurrent`), kein Netzwerk
  (`QNetwork`), keine Subprozesse (`QProcess`) — also keine browser-inkompatiblen
  APIs im Kern. Single-threaded WASM reicht.
- Persistenz ist ein eigenes Binaerformat (`ved_binary_*`) ohne OS-Abhaengigkeit —
  laeuft in-memory unveraendert.

Die Arbeit konzentriert sich fast vollstaendig auf den App-Layer (`src/app/`,
v.a. `MainWindow.cpp` und `QtVecFontProviders.cpp`) und das Build-Setup — **nicht**
auf den Fachkern.

## Ziel

Ein im Browser lauffaehiger OpenVED-Build, der Zeichnen, Bearbeiten, Laden und
Speichern von Dokumenten sowie PDF-Export unterstuetzt, ausgeliefert als
statisch gehostetes Web-Bundle (GitHub Pages) ueber die bestehende CI-Pipeline.

## Nicht-Ziele

- Keine Qt-Abhaengigkeit im `ved_core` (harter Architektur-Grundsatz bleibt).
- Kein nativer Druckdialog im Browser — Druck laeuft ueber PDF-Export.
- Kein Multithread-WASM (kein SharedArrayBuffer/COOP-COEP-Zwang) im ersten
  Durchstich.
- Kein Browser-Font-Zugriff ueber die Local Font Access API
  (`queryLocalFonts`) — wegen Chromium-only + Permission-Prompt + fehlender
  Firefox/Safari-Unterstuetzung bewusst **verworfen**.
- Keine Ablösung der nativen Desktop-Builds — WASM ist ein *zusaetzlicher*
  Distributionspfad.

## Bestandsaufnahme / Machbarkeit

### WASM-freundlich (unveraendert uebernehmbar)

| Aspekt | Befund |
|---|---|
| `ved_core` | 0 Qt-Verstoesse, reines C++23 — kompiliert mit Emscripten |
| Rendering | Abstraktion `TDGraphicEngine`; QPainter nur in `ved_qt` (Qt-WASM voll) |
| Qt-Module | nur `Widgets` (+ `Gui`) im Kern; `PrintSupport` isoliert |
| Nebenlaeufigkeit | keine Threads/Netzwerk/Subprozesse |
| Persistenz | eigenes Binaerformat, in-memory-faehig |
| FreeType/HarfBuzz | Emscripten-Ports vorhanden; Qt-WASM bringt FreeType mit |

### WASM-Baustellen (Aufwand)

1. **`Qt PrintSupport`** (`MainWindow.cpp`) — in Qt for WebAssembly **nicht
   verfuegbar**. `QPrinter`/`QPrintDialog` (`MainWindow.cpp:985-992`) fallen weg.
   `QPdfWriter` (`:928`) ist Teil von QtGui und WASM-tauglich, muss aber in einen
   Speicher-Buffer statt in eine Datei schreiben.
2. **Datei oeffnen/speichern** (`MainWindow.cpp:430,472,910`) — kein
   Browser-Dateisystem. `QFileDialog::getOpenFileName/getSaveFileName` muessen auf
   die async-APIs `QFileDialog::getOpenFileContent()` / `saveFileContent()`
   umgestellt werden.
3. **Fonts** (`QtVecFontProviders.cpp`) — Systemfont-Scan (`C:/Windows/Fonts`,
   `~/Library/Fonts`, `QStandardPaths::FontsLocation`, `:106-122`) ist im Browser
   wirkungslos. Loesung: mitgeliefertes `.vfn`-Basis-Bundle + optionaler lokaler
   Font-Server (siehe Step 4).
4. **`QSettings` / `QStandardPaths`** (`MainWindow.cpp`) — laeuft in Qt-WASM ueber
   IndexedDB (async/verzoegerte Persistenz). Meist unkritisch, "zuletzt geoeffnet"
   ggf. anpassen.
5. **Build-System** — `CMakeLists.txt` (23 KB) enthaelt MSVC-/Windows-Kit-Logik
   und verzweigte FreeType/HarfBuzz-Findung; braucht einen sauberen WASM-Pfad
   (Emscripten-Ports statt find-Logik).

---

## Step 1: Emscripten- und Qt-for-WebAssembly-Toolchain

### Was
- Emscripten-SDK (`emsdk`) installieren, Version passend zur Qt-Version festnageln
  (Qt 6.8 LTS ↔ emsdk-Version laut Qt-Doku).
- Qt for WebAssembly installieren (`wasm_singlethread`) via `aqtinstall` oder
  Qt-Online-Installer.
- Neues CMake-Preset `wasm` mit
  `CMAKE_TOOLCHAIN_FILE=<Qt>/wasm_singlethread/lib/cmake/Qt6/qt.toolchain.cmake`
  (das intern die Emscripten-Toolchain einbindet).
- `CMakeLists.txt` WASM-tauglich machen:
  - MSVC-/Windows-Kit-Bloecke hinter `if(NOT EMSCRIPTEN)` bzw. `if(WIN32)` gaten.
  - FreeType/HarfBuzz unter Emscripten ueber die Emscripten-Ports beziehen
    (`-sUSE_FREETYPE=1`, `-sUSE_HARFBUZZ=1`) oder Qt-mitgeliefertes FreeType
    nutzen; die find-/vendor-Logik nur im nativen Zweig lassen.
  - `ved_copy_qt_runtime()` (Windows-DLL-Kopie) nur nativ.
- App-Target: `qt_add_executable` erzeugt unter WASM automatisch `.html`, `.js`,
  `.wasm`, `qtloader.js`.

### Tests
- [ ] `ved_core` kompiliert vollstaendig unter Emscripten (Bibliotheks-Target).
- [ ] `ved_qt_app` linkt zu einem `.wasm` + `.html`-Bundle.
- [ ] Bundle laedt in Chrome und Firefox, MainWindow erscheint, leeres Blatt
      sichtbar.
- [ ] Native Desktop-Builds (macOS/Windows) bauen unveraendert weiter.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 2: PrintSupport entkoppeln — Druck ueber PDF-Export

### Was
- `QPrinter`/`QPrintDialog`/`#include <QtPrintSupport/...>` aus `MainWindow.cpp`
  hinter `#ifndef Q_OS_WASM` (bzw. `#if !defined(__EMSCRIPTEN__)`) kapseln oder
  in eine plattformbedingte Datei auslagern.
- `PrintSupport` in `CMakeLists.txt` nur im nativen Zweig linken (nicht unter
  Emscripten).
- `QPdfWriter`-Export so umbauen, dass er in einen `QByteArray`/`QBuffer`
  schreibt statt in eine Datei; das Ergebnis geht ueber `saveFileContent()`
  (Step 3) als Browser-Download raus.
- Im WASM-Build tritt der PDF-Export an die Stelle des Druckens (Nutzer druckt
  aus dem PDF-Viewer / Browser-Print-Dialog des heruntergeladenen PDFs).

### Tests
- [ ] WASM-Build linkt ohne `Qt6::PrintSupport`.
- [ ] PDF-Export erzeugt im Browser eine korrekte, downloadbare PDF (gleiches
      Koordinaten-Mapping wie nativ, vgl. `closed_stories/story_pdf_print_export.md`).
- [ ] Nativer Build behaelt Druck **und** PDF-Export unveraendert.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 3: Datei-I/O auf async Browser-APIs

### Was
- Oeffnen (`MainWindow.cpp:430`): unter WASM `QFileDialog::getOpenFileContent()`
  nutzen (liefert Dateiname + `QByteArray` via Callback); Bytes direkt in den
  bestehenden `VEDBinaryReader`-Ladepfad geben (kein Umweg ueber Dateipfad).
- Speichern (`:472`, `:910`): unter WASM `QFileDialog::saveFileContent()` mit den
  serialisierten Bytes (loest Browser-Download aus). Native Pfade bleiben ueber
  `getSaveFileName` erhalten.
- Ladepfade so faktorisieren, dass Kern-Serialisierung (Core) von der
  Datei-Quelle (Pfad nativ vs. Bytes WASM) entkoppelt ist — moeglichst eine
  gemeinsame `loadFromBytes()` / `saveToBytes()`-Schnittstelle.

### Tests
- [ ] Im Browser: Dokument oeffnen per Datei-Upload laedt korrekt.
- [ ] Im Browser: Speichern loest Download der `.ved`-Datei aus, die nativ
      wieder ladbar ist (Round-Trip).
- [ ] Nativer Datei-Dialog unveraendert.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 4: Font-Strategie fuer WASM (und wiederverwendbar)

Browser haben keinen Zugriff auf die rohen TTF-Bytes installierter Systemfonts
(Canvas/CSS liefert nur gerenderte Pixel, nicht die Outlines, die OpenVED fuer
Text→Kurven braucht). Die Local Font Access API ist wegen ihrer
Einschraenkungen verworfen. Stattdessen zwei Wege — ein **immer verfuegbares
Basis-Bundle** plus ein **optionaler lokaler Font-Server**.

### 4a: TTF→VFN-Konverter (Story 15)

Das Konverter-CLI `ved_font_converter` wird in **`story_15_font_converter_tool.md`**
definiert und umgesetzt (Entkopplung der Konvertierungslogik aus
`QtVecFontProviders.cpp`, VFN-Schreibpfad, CLI). Diese Story konsumiert nur das
fertige Tool und dessen `.vfn`-Ausgabe — hier wird nichts am Konverter selbst
mehr entwickelt.

### 4b: Basis-Font-Bundle im `.vfn`-Format anlegen

**Was:**
- Mit dem Konverter aus Story 15 ein Set **frei lizenzierter** Fonts nach `.vfn`
  wandeln — Liberation-Familie (metrik-kompatibler, freier Ersatz fuer die
  proprietaeren MS-Kernschriften Arial/Times New Roman/Courier New), optional
  DejaVu Sans fuer breitere Unicode-Abdeckung.
- In Projektstruktur ablegen und als Qt-Ressource einbinden:
  ```
  src/app/resources/font/
    wps_default.vfn              (bereits vorhanden)
    liberation_sans.vfn         (Ersatz Arial)
    liberation_sans_bold.vfn
    liberation_serif.vfn        (Ersatz Times New Roman)
    liberation_mono.vfn         (Ersatz Courier New)
    dejavu_sans.vfn             (optional, Unicode-Breite)
  ```
- `.qrc`-Eintraege ergaenzen. Bundle ist damit in **jedem** Build (nativ + WASM)
  eingebettet und deterministisch verfuegbar.
- Lizenz-Compliance laeuft zentral ueber `licenses/` + `THIRD_PARTY_LICENSES.md`
  (siehe `plan_installer_...` Step 6), nicht ueber eine separate Font-Datei.

**Tests:**
- [ ] Alle gebuendelten `.vfn` laden im nativen Build ueber den Builtin-Provider.
- [ ] Im WASM-Build steht die Basis-Auswahl ohne Netzwerk/Server zur Verfuegung.

### 4c: Multi-VFN Builtin-Provider

**Was:**
- `TDBuiltinVfnFontProvider` von "nur `wps_default`" auf eine registrierte Liste
  gebuendelter `.vfn` erweitern (Font-ID/DisplayName/Resource-Pfad je Eintrag).
- `MainWindowTextDock.cpp:609` (Provider-Registrierung) entsprechend anpassen.
- Font-Auswahl-UI zeigt die gebuendelten Fonts.

**Tests:**
- [ ] Font-Auswahl listet alle gebuendelten Fonts, jeder ist auswaehlbar und
      wird korrekt gezeichnet.

### 4d: Lokaler Font-Server (optional, Systemfonts im Browser)

**Was:**
- Kleine lokale Server-App (z.B. Python/FastAPI), die auf einem festen
  localhost-Port eine REST-API bereitstellt:
  - `GET /fonts` → Liste verfuegbarer Systemfonts (ID, Name, Style).
  - `GET /font/{id}` → rohe TTF/OTF-Bytes **oder** direkt konvertierte
    `.vfn`-Bytes (serverseitige Konvertierung wiederverwendet den Konverter aus
    4a).
- Neuer `IVecFontProvider` in der App: `TDLocalServerFontProvider`, der beim
  Start `GET /fonts` probiert (kurzer Timeout). Erreichbar → Systemfonts werden
  gelistet und lazy per `GET /font/{id}` geladen; nicht erreichbar → still
  ignorieren, nur Basis-Bundle aktiv.
- HTTP im Browser ueber JS `fetch` (Emscripten/`emscripten::val`); nativ ueber
  denselben Provider mit `fetch`-Aequivalent oder gekapselt.

**Haken (dokumentieren):**
- **Mixed Content:** Eine ueber HTTPS gehostete WASM-Seite (GitHub Pages) darf
  nicht auf `http://localhost:port` zugreifen. Optionen: lokaler Server bietet
  HTTPS mit lokalem Zertifikat, ODER die WASM-App wird selbst lokal ueber
  `http://localhost` ausgeliefert (der Server hostet auch die App), ODER
  Server-Nutzung nur im lokalen/gepackten Kontext.
- **CORS:** Server muss die Origin der WASM-App per CORS-Header erlauben.
- Serverseitig: Konverter aus 4a wiederverwenden, damit der Browser nur `.vfn`
  liest und kein TTF/FreeType zur Laufzeit braucht.

**Tests:**
- [ ] Bei laufendem Server erscheinen Systemfonts in der Auswahl und werden
      korrekt gezeichnet.
- [ ] Bei nicht laufendem Server startet die App ohne Fehler, nur Basis-Bundle
      aktiv (kein Blockieren, kein Timeout-Hang).
- [ ] Mixed-Content-/CORS-Verhalten ist dokumentiert und reproduzierbar.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 5: QSettings / QStandardPaths unter WASM

### Was
- Pruefen, dass `QSettings` (letztes Dokument, UI-State — `MainWindow.cpp`) unter
  WASM ueber IndexedDB funktioniert; async-Persistenz beruecksichtigen.
- "Zuletzt geoeffnetes Dokument" (`:247`) im Browser sinnvoll behandeln
  (Dateipfade existieren nicht — ggf. deaktivieren oder auf zuletzt in IndexedDB
  abgelegtes Dokument umstellen).
- `QStandardPaths::FontsLocation`-Nutzung im WASM-Zweig deaktivieren (Step 4
  uebernimmt Fonts).

### Menue-Switch "Convert System Fonts" (aus Story 15 Step 5)

Der in `story_15_font_converter_tool.md` (Step 5) eingefuehrte Menue-Umschalter
"Convert System Fonts" ist bereits **WASM-aware implementiert**:
- Unter `#if defined(Q_OS_WASM)` wird die Action `setEnabled(false)` + unchecked,
  und `MainWindow::systemFontsEnabled()` liefert immer `false`.
- Im Browser gibt es keine installierten Systemfonts zum Scannen/Konvertieren —
  der Switch bleibt daher deaktiviert und ausgegraut; es sind nur die
  gebuendelten `.vfn`-Fonts (bzw. spaeter der lokale Font-Server, Step 4d)
  verfuegbar.
- QSettings-Key `fonts/convertSystemFonts` wird auf WASM nicht geschrieben.

### Tests
- [ ] UI-State/Settings ueberleben ein Reload der Browser-Seite.
- [ ] Kein Absturz/Fehler durch nicht existente Pfade im Browser.
- [ ] "Convert System Fonts" ist im WASM-Build sichtbar, aber ausgegraut/disabled.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 6: Deployment — WASM-Web-Distribution

Ergaenzt `plan_installer_build_github_distribution.md` um einen dritten
Distributionspfad (Details dort in einem neuen "Step 5: WebAssembly").

### Was
- `scripts/build-wasm.sh`: emsdk aktivieren, mit dem `wasm`-Preset configuren +
  bauen, Bundle (`.html/.js/.wasm/qtloader.js` + Ressourcen) nach
  `build/wasm/dist/` sammeln.
- GitHub-Actions-Job `build-wasm` (ubuntu-latest): emsdk + Qt-for-WASM
  installieren, bauen, Bundle als Artifact hochladen und/oder per
  `actions/deploy-pages` auf **GitHub Pages** veroeffentlichen.
- README: Link zur Live-Web-Version ergaenzen.

### Tests
- [ ] CI baut das WASM-Bundle reproduzierbar.
- [ ] Veroeffentlichtes Bundle laedt ueber die GitHub-Pages-URL und ist bedienbar.

### Log
_(nach Umsetzung ausfuellen)_

---

## Akzeptanzkriterien

- `ved_core` bleibt Qt-frei; nativer Desktop-Build (macOS/Windows) unveraendert
  lauffaehig inkl. Druck.
- WASM-Bundle laedt in Chrome und Firefox; Zeichnen/Selektieren/Bearbeiten
  funktioniert.
- Dokument oeffnen (Upload) und speichern (Download) im Browser mit
  Round-Trip-Treue zur nativen Datei.
- PDF-Export im Browser erzeugt korrekte, downloadbare PDF; kein `PrintSupport`
  im WASM-Link.
- Basis-Font-Bundle (`.vfn`) ist eingebettet und ohne Netzwerk verfuegbar.
- Optionaler lokaler Font-Server liefert bei Verfuegbarkeit Systemfonts; Abwesen-
  heit blockiert den Start nicht.
- CI veroeffentlicht das WASM-Bundle auf GitHub Pages.

## Reihenfolge

0. **Story 15** (`ved_font_converter`-CLI) — Voraussetzung fuer Step 4.
1. Step 1 — Toolchain + WASM-Build steht (leeres Fenster laedt).
2. Step 2 — PrintSupport entkoppeln (Build linkt sauber).
3. Step 3 — Datei-I/O async (Laden/Speichern im Browser).
4. Step 4b/4c — Basis-Bundle (via Story-15-CLI) + Multi-VFN-Provider (Text im Browser).
5. Step 5 — Settings/Paths.
6. Step 4d — Lokaler Font-Server (optionaler Komfort, nach dem Durchstich).
7. Step 6 — CI/GitHub-Pages-Deployment.

## Bezug zu bestehenden Stories/Plaenen

- **`story_15_font_converter_tool.md`** — **Voraussetzung**; liefert das
  `ved_font_converter`-CLI und die konvertierten `.vfn`-Fonts fuer Step 4.
- `plan_installer_build_github_distribution.md` — wird um "Step 5: WebAssembly"
  (build-wasm-Skript + Pages-Job) ergaenzt; dessen WASM-Teil ist von dieser
  Story 16 abhaengig.
- `story_font_provider_truetype_converter_plugin.md` — liefert die
  Konvertierungs-Grundlage; das Build-Zeit-CLI (Story 15) und ein spaeteres
  Laufzeit-Plugin teilen sich dieselbe entkoppelte Konvertierungs-Einheit.
- `story_document_font_packaging.md` — dieselbe VFN-/Provider-Maschinerie; das
  WASM-Basis-Bundle und ein spaeteres Dokument-Font-Embedding ergaenzen sich.
- `closed_stories/story_pdf_print_export.md` — Basis fuer den PDF-Export, der
  unter WASM den Druck ersetzt.
