# Story 16: OpenVED als WebAssembly-App im Browser

Datum: 2026-07-20
Status: offen

**Zu Story 15:** `story_15_font_converter_tool.md` (`ved_font_converter`-CLI) ist
seit der Font-Strategie-Aenderung in Step 4 **keine Voraussetzung** mehr — das
WASM-Basis-Bundle liefert rohe TTF und wandelt sie **zur Laufzeit** nach Vec-Font.
Story 15 bleibt als eigenstaendiges Tool und fuer den optionalen Font-Server
(Step 4b) relevant.

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
   `~/Library/Fonts`, `QStandardPaths::FontsLocation`) ist im Browser wirkungslos.
   Loesung: mitgeliefertes **TTF-Basis-Bundle** (zur Laufzeit nach Vec-Font
   gewandelt) + optionaler lokaler Font-Server (siehe Step 4).
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
- [x] `ved_core` kompiliert vollstaendig unter Emscripten (Bibliotheks-Target).
- [x] `ved_qt_app` linkt zu einem `.wasm` + `.html`-Bundle.
- [x] Bundle laedt (in **Chrome** bestaetigt), MainWindow erscheint, Zeichnen +
      Text funktionieren. (Firefox noch nicht separat geprueft.)
- [x] Native Desktop-Builds: CMake-Aenderungen sind alle `EMSCRIPTEN`-gegated;
      nativer Configure verifiziert gruen (voller nativer Rebuild noch offen).

### Log

**umgesetzt am 2026-07-28/29** (Branch `story_16_webassembly_step_1`):

**Toolchain — mit Upgrade auf Qt 6.11.**
- Start mit **Qt 6.9.3 wasm_singlethread** + **Emscripten 3.1.70** (per Qt-Doku
  gekoppelt), Host-Qt = Homebrew `qtbase 6.9.3` (`QT_HOST_PATH`), installiert via
  `emsdk` + `aqtinstall` (in venv `~/.aqt-venv`, wegen PEP-668).
- **Upgrade auf Qt 6.11.1** (Grund: Combobox-Bug unten): **Emscripten 4.0.7** +
  Qt **6.11.1 wasm_singlethread** + Qt **6.11.1 Desktop (clang_64)** als Host-Qt
  (Homebrew 6.9.3 passt als Host nicht mehr zur 6.11-Target-Version). Damit
  weicht die Story von der urspruenglichen Annahme "Qt 6.8 LTS" ab — bewusst
  auf **6.11.1**.
- WASM-Configure:
  `-DCMAKE_TOOLCHAIN_FILE=~/Qt/6.11.1/wasm_singlethread/lib/cmake/Qt6/qt.toolchain.cmake`
  `-DQT_HOST_PATH=~/Qt/6.11.1/macos`. (Pfade aktuell nur manuell — ein
  `scripts/build-wasm.sh` folgt in Step 6.)

**CMake-Anpassungen (`CMakeLists.txt`, alle `EMSCRIPTEN`-gegated):**
- FreeType/HarfBuzz unter Emscripten ueber die **Ports** (`-sUSE_FREETYPE=1`,
  `-sUSE_HARFBUZZ=1` als compile+link options); native find-/vendor-Logik bleibt
  unberuehrt. `ved_core` nutzt FreeType/HarfBuzz direkt — die Ports liefern
  Header + Libs.
- `ved_qt_gengine_outline_tests` (Qt/QPainter-Test) unter WASM ausgeschlossen:
  linkt `libqwasm` (embind-Symbole, die ein CLI-Test nicht bereitstellt) und ist
  headless sinnlos. Die uebrigen `ved_core_*`-Tests bauen als `.wasm`.
- **Asyncify** fuers App-Target: `-sASYNCIFY -sASYNCIFY_STACK_SIZE=131072`.
  Noetig, weil modale Dialoge (`QDialog`/`QMessageBox::exec()`) eine
  verschachtelte Event-Loop nutzen, die in single-threaded WASM ohne Asyncify
  nicht laeuft → Dialoge erschienen leer und liessen sich nicht wegklicken.
  Binary dadurch ~17 MB → ~25 MB. (Multithread-WASM bleibt Nicht-Ziel.)

**Combobox-in-Dialog-Bug → Grund fuers 6.11-Upgrade:**
- Unter **Qt 6.9.3** (auch mit Asyncify): Comboboxen in Dialogen (New-Dialog:
  Unit/Format/Orientation) oeffneten das Popup, aber **Maus-Auswahl** ging nicht
  — nur **Tastatur**. Im Hauptfenster funktionierten Comboboxen normal. Bekannter,
  ungeloester Qt-WASM-Bug (Popup-Maus-Routing ueber Dialogen; Wurzel:
  `QDialog::exec()` in WASM, QTBUG-90989; Qt-Forum-Thread 161427).
- **Unter Qt 6.11.1 behoben** — Maus-Auswahl in Dialog-Comboboxen funktioniert.

**Funktioniert im Browser (Qt 6.11.1, vom User abgenommen):**
Laden, Zeichnen, Text schreiben, modale Dialoge (Inhalt sichtbar + bedienbar),
**Speichern** (Qt-6.11-WASM mappt `QFileDialog::getSaveFileName` intern auf einen
Browser-Download → nativer Code laeuft ohne Umbau).

**Noch offen / fuer Folge-Steps:**
- PDF-Export und Print im Browser pruefen (Step 2) — evtl. durch Qt 6.11 schon
  teilweise gedeckt.
- Öffnen (Upload) verifizieren (Step 3).
- Reproduzierbares `scripts/build-wasm.sh` (Step 6).
- Voller nativer Rebuild + Abnahme (Regression durch CMake-Aenderungen
  ausschliessen).
- Firefox-Test.

---

## Step 1b: FreeType/HarfBuzz-Versionskonsistenz (native ↔ WASM)

Datum: 2026-07-29

**Problem (entdeckt nach Step 1):** WASM- und native-Build rendern denselben Text
(z.B. Amiri/Persisch) sichtbar unterschiedlich — bereits im Editor, nicht nur im
PDF-Export. Ursache: Die Font-Bibliotheken stammen aus verschiedenen Quellen mit
verschiedenen Versionen.

| Bibliothek | Native (Homebrew) | WASM (Emscripten-Port) |
|---|---|---|
| FreeType | 2.14.3 | 2.13.3 |
| HarfBuzz | **12.3.2** | **3.2.0** |

HarfBuzz 3.2.0 (Emscripten-Port, ~2021) ist gegenueber 12.3.2 (nativ) neun Major-
Versionen alt → abweichendes Shaping (Buchstabenverbindung/-form), besonders bei
Arabisch/Persisch. Die FreeType-Differenz erklaert zusaetzliche Outline-Unter-
schiede (und die stark abweichenden PDF-Groessen: 26 KB nativ vs 194 KB WASM fuer
dieselbe Datei `Work/Lukas.ved`, Amiri-Text). Nativ (aktuelle Bibliotheken) ist
das korrektere Ergebnis.

**Ziel:** Identisches Text-Rendering auf allen Plattformen.

### Was
- FreeType + HarfBuzz **nicht** mehr aus Homebrew (native) bzw. Emscripten-Port
  (WASM) beziehen, sondern in **einer festen Version fuer alle Plattformen** via
  **`FetchContent`** aus Source bauen (CMake laedt die Quellen und kompiliert sie
  fuer das jeweilige Ziel — nativer Compiler bzw. Emscripten).
- Versionen pinnen: **HarfBuzz 12.x** (aktuelles Shaping, = bisheriges natives
  Verhalten) + **FreeType 2.14.x**. Als gepinnte Git-Tags/Releases.
- Abhaengigkeit HarfBuzz ↔ FreeType korrekt aufsetzen (HarfBuzz mit FreeType-
  Unterstuetzung bauen, richtige Link-Reihenfolge).
- Den Emscripten-Ports-Zweig (`-sUSE_FREETYPE`/`-sUSE_HARFBUZZ`) aus Step 1
  ersetzen; den nativen find-/Homebrew-Zweig ebenfalls auf FetchContent umstellen.
- Als Alternative (Offline-Build) vendored `third_party/` dokumentieren.

### Tests
- [x] Nativer Build gruen (FetchContent-Libs aus Source; `build/release-ft`).
- [x] WASM-Build gruen (FreeType + HarfBuzz mit Emscripten gebaut).
- [x] `Work/Lukas.ved`: WASM rendert jetzt **wie nativ** (Amiri) — vom User
      visuell im Editor bestaetigt. (Objektiver PDF-Pixelvergleich optional.)
- [ ] Stichprobe weiterer Skripte (Latin, Kyrillisch, Griechisch, Hebraeisch,
      Arabisch/Persisch): noch offen (bisher Amiri/Persisch verifiziert).

### Log

**umgesetzt am 2026-07-29** (Branch `story_16_webassembly_step_2`):
- `CMakeLists.txt`: FreeType/HarfBuzz-Beschaffung komplett auf **`FetchContent`**
  umgestellt — ersetzt sowohl den nativen Homebrew-/find-Zweig als auch den
  Emscripten-Ports-Zweig aus Step 1. Gepinnt: **FreeType `VER-2-14-1`** +
  **HarfBuzz `12.3.2`** (`GIT_SHALLOW`), fuer **alle** Plattformen dieselbe
  Version → identisches Rendering.
- HarfBuzz **mit** FreeType-Support (`HB_HAVE_FREETYPE=ON`; die App nutzt
  `hb_ft_font_create`). HarfBuzz' `find_package(Freetype)` wird per gesetzten
  `FREETYPE_*`-Variablen auf die FetchContent-FreeType umgeleitet (kein System-
  Install noetig).
- FreeType schlank: `FT_DISABLE_HARFBUZZ/PNG/ZLIB/BZIP2/BROTLI=ON` (vermeidet
  FreeType↔HarfBuzz-Zyklus; TrueType-Outlines unberuehrt). Alles static.
- Ergebnis: nativer Build (`build/release-ft`, App 5,2 MB) **und** WASM-Build
  (`build/wasm`, `libfreetype.a`+`libharfbuzz.a` via Emscripten) gruen; WASM-Text
  jetzt = nativ (vorher HarfBuzz 3.2.0 → jetzt 12.3.2).
- **Trade-off:** erster Configure klont die Repos; Build-Zeit laenger (Libs aus
  Source). Fuer Offline-Builds bleibt vendored `third_party/` als Alternative
  moeglich (nicht implementiert).
- **Offen:** Stichprobe weiterer Skripte; `build-macos.sh`/`build-windows.ps1`
  brauchen kein Homebrew-/vcpkg-FreeType/HarfBuzz mehr (kann spaeter entschlackt
  werden); CI-Anpassung analog.

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

## Step 4: Font-Strategie fuer WASM — TTF-Bundle + Laufzeit-Konvertierung

Browser haben keinen Zugriff auf die rohen TTF-Bytes installierter Systemfonts
(Canvas/CSS liefert nur gerenderte Pixel, nicht die Outlines, die OpenVED fuer
Text→Kurven braucht). Die Local Font Access API ist wegen ihrer Einschraenkungen
verworfen. Stattdessen zwei Wege — ein **immer verfuegbares Basis-Bundle** plus
ein **optionaler lokaler Font-Server**.

**Strategie-Aenderung (2026-07-29):** Das Basis-Bundle wird als Satz **roher TTF**
mitgeliefert und **zur Laufzeit** nach Vec-Font gewandelt (FreeType/HarfBuzz +
`ttf_to_vecfont`) — **nicht** mehr als vorab konvertierte `.vfn` eingecheckt. Das
ist verlaesslich moeglich, weil FreeType/HarfBuzz seit **Step 1b** in **jedem**
Build (nativ + WASM) via FetchContent in **derselben** Version fest eingebaut sind
→ die Laufzeit-Konvertierung liefert auf allen Plattformen identische Outlines.
Damit entfaellt ein Vorab-Konvertierungsschritt und das Einchecken von `.vfn`;
Story 15 (`ved_font_converter`-CLI) ist fuer dieses Bundle **keine Voraussetzung**
mehr. `wps_default.vfn` bleibt als Default-Font im VFN-Format bestehen.

**Bereits im Code vorhanden (mit Step 1b eingezogen):**
- `CMakeLists.txt:355-357` bettet `resources/font/*.ttf` **und** `*.vfn` als
  Qt-Ressource unter `:/ved/font` ein — die TTFs (Amiri, Liberation Sans/Serif/
  Mono, Noto Sans, Noto Sans Hebrew) liegen bereits gebuendelt vor.
- `TDQtSystemFontProvider` (`QtVecFontProviders.cpp:250-268`) indiziert die
  gebuendelten TTFs unter `:/ved/font` **immer** (unabhaengig vom optionalen
  System-Scan) und wandelt sie zur Laufzeit — der Multi-Font-Provider existiert
  damit bereits; ein separater "Multi-VFN Builtin-Provider" entfaellt.
- `TDBuiltinVfnFontProvider` (`MainWindowTextDock.cpp:623`) bleibt fuer
  `wps_default.vfn` (Default-Font).

### 4a: Basis-Bundle finalisieren

**Was:**
- Font-Auswahl fuers Bundle festzurren (**frei lizenziert**): Liberation-Familie
  (metrik-kompatibler Ersatz fuer Arial/Times New Roman/Courier New), Amiri
  (Arabisch/Persisch), Noto Sans + Noto Sans Hebrew (Unicode-Breite). Ggf. Bold-/
  Kursiv-Schnitte ergaenzen.
- Bundle-Groesse im Blick behalten (zaehlt zum WASM-Download).
- Lizenz-Compliance zentral ueber `licenses/` + `THIRD_PARTY_LICENSES.md`
  (siehe `plan_installer_...` Step 6).

**Tests:**
- [ ] Alle gebuendelten TTF laden im **nativen** Build und werden korrekt gezeichnet.
- [x] Im **WASM**-Build steht dieselbe Auswahl ohne Netzwerk/Server zur Verfuegung
      (vom User geprueft; Rendering-Identitaet zu nativ bisher fuer Amiri via
      Step 1b bestaetigt, restliche Fonts als Stichprobe offen).
- [x] Font-Auswahl-UI listet alle gebuendelten Fonts (vom User im WASM-Build
      bestaetigt); Auswaehlbarkeit/korrektes Zeichnen je Font als Stichprobe offen.

### 4b: Lokaler Font-Server (optional, Systemfonts im Browser)

**Was:**
- Kleine lokale Server-App (z.B. Python/FastAPI), die auf einem festen
  localhost-Port eine REST-API bereitstellt:
  - `GET /fonts` → Liste verfuegbarer Systemfonts (ID, Name, Style).
  - `GET /font/{id}` → rohe TTF/OTF-Bytes; der Client wandelt zur Laufzeit
    (wie beim Basis-Bundle) — keine serverseitige VFN-Konvertierung noetig.
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
- Serverseitig genuegt das Ausliefern roher TTF/OTF — die Konvertierung nach
  Vec-Font passiert im Client (FreeType/HarfBuzz sind im WASM-Build vorhanden).

**Tests:**
- [ ] Bei laufendem Server erscheinen Systemfonts in der Auswahl und werden
      korrekt gezeichnet.
- [ ] Bei nicht laufendem Server startet die App ohne Fehler, nur Basis-Bundle
      aktiv (kein Blockieren, kein Timeout-Hang).
- [ ] Mixed-Content-/CORS-Verhalten ist dokumentiert und reproduzierbar.

### Log

**Strategie-Aenderung + Teil-Abnahme am 2026-07-29:**
- Font-Strategie umgestellt: **TTF-Bundle + Laufzeit-Konvertierung** statt Vorab-
  `.vfn` (Details oben im Step-Kopf). Der tragende Mechanismus wurde faktisch
  schon mit **Step 1b** eingezogen (TTF-Ressourcen unter `:/ved/font` +
  `TDQtSystemFontProvider` indiziert/wandelt zur Laufzeit).
- **Vom User geprueft (WASM):** Die Font-Auswahl im Browser zeigt genau das
  gebuendelte Set (Amiri, Liberation Sans/Serif/Mono, Noto Sans, Noto Sans
  Hebrew) — ohne Netzwerk/Server. Damit ist der Kern von **4a** im WASM-Build
  bestaetigt.
- **Offen:** nativer Build als Gegenprobe (Test 1); Rendering-Stichprobe je Font
  (Latin/Kyrillisch/Griechisch/Hebraeisch/Arabisch — bisher nur Amiri via 1b);
  finales Festzurren der Font-Auswahl inkl. evtl. Bold-/Kursiv-Schnitte und
  Lizenz-Eintraege (`THIRD_PARTY_LICENSES.md`).
- **4b (Font-Server):** noch nicht begonnen (optional, nach dem Durchstich).

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
- [x] `scripts/build-wasm.sh` erzeugt lokal ein vollstaendiges Bundle
      (`build/wasm/dist/`: index.html + OpenVED.js/.wasm + qtloader.js).
- [ ] CI baut das WASM-Bundle reproduzierbar (`build-wasm`-Job) — CI-Lauf offen.
- [ ] Veroeffentlichtes Bundle laedt ueber die GitHub-Pages-URL und ist bedienbar
      (setzt voraus: Pages in den Repo-Settings auf "GitHub Actions" gestellt).

### Log

**umgesetzt am 2026-07-29/30** (Branch `story_16_webassembly_step_3`):
- `scripts/build-wasm.sh`: parametrisiert ueber `EMSDK`/`QT_WASM`/`QT_HOST`
  (macOS-Defaults fuer lokal), Configure mit der Qt-wasm-Toolchain +
  `QT_HOST_PATH`, Build `ved_qt_app`, Bundle nach `build/wasm/dist/` (inkl.
  `index.html` = Kopie der generierten `OpenVED.html` fuer den Pages-Root).
  **Lokal getestet** (Bundle erzeugt, ~25 MB `.wasm`).
- `.github/workflows/release.yml`: zwei neue Jobs —
  - **`build-wasm`** (ubuntu-22.04): `setup-emsdk@v14` (4.0.7) +
    `install-qt-action` (Qt 6.11.1 `wasm_singlethread` **und** Linux-Host-Qt
    fuer moc/rcc) → `build-wasm.sh` → `upload-pages-artifact`.
  - **`deploy-pages`**: `actions/deploy-pages@v4` (permissions `pages: write`,
    `id-token: write`, environment `github-pages`) → live-URL.
  Laeuft bei Tag-Push **und** `workflow_dispatch` (Web-Version unabhaengig von
  Desktop-Releases aktualisierbar).
- **Voraussetzung (einmalig, durch User):** Repo → Settings → Pages → Source
  "GitHub Actions". Ohne das schlaegt `deploy-pages` fehl.
- Offen: erster CI-Lauf verifizieren; Live-URL (`https://rasegi.github.io/OpenVED/`)
  testen; README-Link ergaenzen.

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
- TTF-Basis-Bundle ist eingebettet, wird zur Laufzeit nach Vec-Font gewandelt und
  ist ohne Netzwerk verfuegbar (nativ + WASM identisches Rendering).
- Optionaler lokaler Font-Server liefert bei Verfuegbarkeit Systemfonts; Abwesen-
  heit blockiert den Start nicht.
- CI veroeffentlicht das WASM-Bundle auf GitHub Pages.

## Reihenfolge

0. **Story 15** (`ved_font_converter`-CLI) — **nicht mehr** Voraussetzung fuer
   Step 4 (Bundle wird zur Laufzeit gewandelt); eigenstaendig / fuer Font-Server.
1. Step 1 — Toolchain + WASM-Build steht (leeres Fenster laedt). **✓ erledigt**
   (auf Qt 6.11.1, mit Asyncify; Zeichnen/Text/Dialoge/Save laufen im Browser).
1b. Step 1b — FreeType/HarfBuzz-Versionskonsistenz (FetchContent), damit WASM
   **identisch** zu nativ rendert. **← naechster Schritt** (`step_2`-Branch).
2. Step 2 — PrintSupport / PDF-Export: unter Qt 6.11 im Browser **bereits
   funktionsfaehig** (PDF-Export + Druck getestet); ggf. nur noch Feinschliff.
3. Step 3 — Datei-I/O: Speichern funktioniert unter Qt 6.11 bereits; Oeffnen
   (Upload) noch verifizieren.
4. Step 4a — TTF-Basis-Bundle finalisieren (Provider-Mechanismus liegt aus Step 1b
   bereits vor; Konvertierung zur Laufzeit).
5. Step 5 — Settings/Paths.
6. Step 4d — Lokaler Font-Server (optionaler Komfort, nach dem Durchstich).
7. Step 6 — CI/GitHub-Pages-Deployment.

## Bezug zu bestehenden Stories/Plaenen

- **`story_15_font_converter_tool.md`** — eigenstaendiges CLI; seit der
  Strategie-Aenderung in Step 4 **keine Voraussetzung** mehr (Bundle wird zur
  Laufzeit gewandelt), bleibt fuer den optionalen Font-Server (Step 4b) relevant.
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
