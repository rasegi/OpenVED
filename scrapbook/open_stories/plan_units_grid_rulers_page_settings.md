# Plan: Core Units, Grid/Ruler Scale und Page Settings

Datum: 2026-05-26
Story: `story_core_units_grid_rulers_page_settings.md`
Konzept: `concept_units_pages_templates_pdf_print.md`

## Architektur-Grundsatz

**VED Core muss Qt-unabhaengig bleiben.**

- Alle fachlichen Typen, Umrechnungen und Berechnungen gehoeren in `ved_core`.
- `ved_core` darf keine Qt-Header einbinden (`QString`, `QPainter`, `QColor` etc.).
- Qt-Code (`ved_qt`, `app`) darf Core-Typen nutzen, aber nie umgekehrt.
- Core verwendet `std::string`, `double`, eigene Structs — kein Qt.

---

## Step A: Core-Datentypen (Units + Formatter)

### Was wird gemacht

Neue Dateien `src/ved_core/main/vec_units.h` und `vec_units.cpp`.

Inhalte:

- `enum class TDVecDisplayUnit` — `RawVed`, `Millimeter`, `Centimeter`, `Inch`
- `struct TDVecUnitSettings` — `realUnitsPerMillimeter` (default 1000.0), `displayUnit` (default Millimeter)
- `class TDVecUnitFormatter` — Umrechnung Real ↔ mm ↔ DisplayUnit, Formatierung als `std::string`

Methoden des Formatters:

| Methode | Logik |
|---|---|
| `ToMillimeters(real)` | `real / realUnitsPerMillimeter` |
| `FromMillimeters(mm)` | `mm * realUnitsPerMillimeter` |
| `ToDisplayUnit(real)` | mm → cm (/10), mm → inch (/25.4), RawVed (identity) |
| `FromDisplayUnit(display)` | inverse |
| `FormatCoordinate(real)` | z.B. `"123.45 mm"`, `"4.86 in"` — 2 Dezimalstellen + Suffix |
| `FormatLength(real)` | wie FormatCoordinate, aber Betrag (`std::fabs`) |

### CMake

- `vec_units.h` und `vec_units.cpp` in `add_library(ved_core STATIC ...)` einfuegen
- Neues Test-Target `ved_core_units_tests`

### Tests: `tests/ved_core_units_tests.cpp`

- [x] ToMillimeters: `1000.0 Real → 1.0 mm` bei default Settings
- [x] FromMillimeters: `1.0 mm → 1000.0 Real`
- [x] ToDisplayUnit Millimeter: identisch zu ToMillimeters
- [x] ToDisplayUnit Centimeter: `10000.0 Real → 1.0 cm`
- [x] ToDisplayUnit Inch: `25400.0 Real → 1.0 in`
- [x] FromDisplayUnit: Roundtrip `FromDisplayUnit(ToDisplayUnit(x)) == x`
- [x] RawVed-Modus: ToDisplayUnit = identity
- [x] FormatCoordinate: korrekte Suffixe (`mm`, `cm`, `in`, `ved`)
- [x] FormatLength: Betragswert (negative Eingabe → positiver Output)
- [x] Nicht-Default realUnitsPerMillimeter (z.B. 500.0)

### Log

- 2026-05-26: `vec_units.h/cpp` erstellt, 14 Tests in `ved_core_units_tests.cpp`, alle bestanden
- Gesamte Test-Suite: 22/22 bestanden

---

## Step B: MeasureScale in Core

### Was wird gemacht

Neue Dateien `src/ved_core/main/vec_measure_scale.h` und `vec_measure_scale.cpp`.

Inhalte:

- `struct TDVecMeasureTick` — `realValue`, `major`, `labeled`, `label`
- `struct TDVecMeasureScale` — `minorStepReal`, `majorStepReal`, `labelStepReal`
- `class TDVecMeasureScaleCalculator` — berechnet Scale und Ticks

Die `niceStepAtLeast`-Logik (aktuell dupliziert in `QVedWidget.cpp:34` und
`vec_graphic_engine_qt.cpp:34`) wird in den Core verschoben als
private/freie Funktion im `.cpp`.

`CalculateForView` bestimmt anhand von `realPerPixel`, Pixel-Minimum und
Grid-Settings die passenden minor/major/label Steps.

`BuildTicks` erzeugt eine Liste von `TDVecMeasureTick` fuer einen sichtbaren
Bereich. Labels kommen vom `TDVecUnitFormatter`.

### CMake

- `vec_measure_scale.h` und `vec_measure_scale.cpp` in `add_library(ved_core STATIC ...)`
- Neues Test-Target `ved_core_measure_scale_tests`

### Tests: `tests/ved_core_measure_scale_tests.cpp`

- [x] CalculateForView bei hohem Zoom (kleines realPerPixel): minor = 1000, major = 10000
- [x] CalculateForView bei niedrigem Zoom (grosses realPerPixel): Steps werden groesser
- [x] CalculateForView bei sehr niedrigem Zoom: groessere Steps
- [x] CalculateForView: minor ist immer <= major (ueber mehrere Zoom-Stufen)
- [x] NiceStepAtLeast: kleine Werte, runde Werte, Zwischenwerte
- [x] BuildTicks: korrekte Anzahl Ticks fuer einen definierten Bereich (11 fuer 0-10000)
- [x] BuildTicks: major-Ticks sind Vielfache von majorStep (3 major in 0-20000)
- [x] BuildTicks: labeled-Ticks haben nicht-leere Labels
- [x] BuildTicks: Labels enthalten mm-Suffix
- [x] BuildTicks: leerer Bereich → leere Liste
- [x] BuildTicks: ungueltigr Step (0) → leere Liste
- [x] Default-Settings ergeben identisches Verhalten wie bisherige Qt-Logik

### Log

- 2026-05-26: `vec_measure_scale.h/cpp` erstellt, `NiceStepAtLeast` aus Qt portiert
- 14 Tests in `ved_core_measure_scale_tests.cpp`, alle bestanden
- Gesamte Test-Suite: 23/23 bestanden

---

## Step C: DocumentSettings im Model

### Was wird gemacht

Neue Dateien `src/ved_core/main/vec_document_settings.h` und `vec_document_settings.cpp`.

Inhalte:

- `enum class TDVecPageOrientation` — `Portrait`, `Landscape`
- `struct TDVecGridSettings` — `majorStepReal` (10000), `subdivisions` (10), `resolutionLimitPixels` (1)
- `struct TDVecPageSettings` — `formatName` ("A4"), `widthReal` (210000), `heightReal` (297000), `orientation`
- `struct TDVecDocumentSettings` — buendelt `TDVecUnitSettings`, `TDVecGridSettings`, `TDVecPageSettings`
- `class TDVecPageFormats` — statische Factory-Methoden: `A4()`, `A3()`, `A5()`, `Letter()`, `Custom()`

Integration in `TDVecModel` (`vec_model.h/cpp`):

- Neuer Member: `TDVecDocumentSettings documentSettings_`
- Getter: `DocumentSettings()`, `UnitSettings()`, `GridSettings()`, `PageSettings()`
- Setter: `SetDocumentSettings(...)`
- Default-Werte reproduzieren exakt das bisherige Verhalten
- `bottomRightArea_` wird konsistent zu PageSettings gehalten

`TDVecModelSnapshot` (`vec_model.h`):

- Erhaelt zusaetzlich `TDVecDocumentSettings documentSettings`
- `CreateSnapshot()` und `RestoreSnapshot()` sichern/laden DocumentSettings mit

Hinweis: Die bisherige Workspace-Hoehe `296985` wird auf `297000` (exaktes A4) korrigiert.

### CMake

- `vec_document_settings.h` und `vec_document_settings.cpp` in `add_library(ved_core STATIC ...)`
- Neues Test-Target `ved_core_document_settings_tests`

### Tests: `tests/ved_core_document_settings_tests.cpp`

- [ ] PageFormats A4 Portrait: `210000 x 297000`
- [ ] PageFormats A4 Landscape: `297000 x 210000`
- [ ] PageFormats A3 Portrait: `297000 x 420000`
- [ ] PageFormats A5 Portrait: `148000 x 210000`
- [ ] PageFormats Letter Portrait: `215900 x 279400` (8.5 x 11 in)
- [ ] Custom: beliebige Werte
- [ ] TDVecModel Default-DocumentSettings = A4/mm/10000-10
- [ ] SetDocumentSettings + Getter-Roundtrip
- [ ] Snapshot: DocumentSettings wird mitgesichert und wiederhergestellt
- [ ] SetDocumentSettings markiert Model als changed

### Log

_(wird bei Umsetzung ausgefuellt)_

---

## Step D: Qt-Anbindung (Grid, Ruler, Statusbar)

### Was wird gemacht

Aenderungen an bestehenden Qt-Dateien. Keine neuen Dateien.

**`vec_graphic_engine.h` (Core):**
- `DrawRulers(long, long, long)` → `DrawRulers(const TDVecMeasureScale&, const TDVecUnitFormatter&)`
- Das ist die einzige Core-Aenderung — die neue Signatur nutzt Core-Typen statt roher Zahlen

**`vec_graphic_engine_qt.cpp` (Qt):**
- `DrawRulers` implementiert die neue Signatur
- Steps und Labels kommen aus `TDVecMeasureScale` und `TDVecUnitFormatter`
- `niceRulerStepAtLeast` und `isMainRulerValue` werden entfernt (jetzt im Core)

**`QVedWidget.cpp` (Qt):**
- Konstanten `kGridDistance`, `kGridSubDivisions`, `kGridResolutionLimit` entfernt
- `kWorkspaceWidth`, `kWorkspaceHeight` entfernt
- `drawGrid()` liest `GridSettings` aus dem Model, nutzt `TDVecMeasureScaleCalculator`
- `drawRulers()` uebergibt `TDVecMeasureScale` + `TDVecUnitFormatter` an GraphicEngine
- `snapPointToGrid()` berechnet Snap-Step aus `GridSettings`
- `niceGridStepAtLeast` entfernt (jetzt im Core)

**`MainWindow.cpp` (Qt):**
- `initializeEditor()`: `SetBottomRightArea` aus `PageSettings().widthReal/heightReal`
- `onMouseCoordinateChanged()`: nutzt `TDVecUnitFormatter` fuer formatierte Koordinaten
- Statusbar zeigt z.B. `"X: 123.45 mm  Y: 67.89 mm"`

**Zugriffspfad:** Qt-Code erreicht DocumentSettings ueber das Model:
`model->DocumentSettings()` / `model->GridSettings()` etc.

### Tests

Kein eigenes Test-Target — Verifikation visuell:

- [ ] App starten: Grid sieht identisch aus wie vorher
- [ ] Ruler zeigt dieselben Ticks und Labels
- [ ] Statusbar zeigt Koordinaten mit Einheit
- [ ] Grid-Snap funktioniert wie vorher
- [ ] Verschiedene Zoom-Stufen: Grid-Aufloesung passt sich an
- [ ] Bestehende `ctest`-Suite laeuft weiterhin durch

### Log

_(wird bei Umsetzung ausgefuellt)_

---

## Step E: Page Bounds anzeigen

### Was wird gemacht

**`QVedWidget.h/cpp` (Qt):**
- Neues Member `bool showPageBounds_ = true`
- Neue Methode `drawPageBounds()`:
  - Zeichnet Rechteck von `{0,0}` bis `{pageSettings.widthReal, pageSettings.heightReal}`
  - Bereich ausserhalb der Page leicht abgedunkelt (halbdurchsichtiges Overlay)
  - Page-Rand als dünne Linie
- Aufruf in `paintEvent` vor `drawGrid()`
- `showPageBounds_` ist View-State — nicht im Dokument gespeichert

### Tests

Kein eigenes Test-Target — Verifikation visuell:

- [ ] Page-Rahmen sichtbar bei `0,0` bis Seitengroesse
- [ ] Bereich ausserhalb der Page ist visuell abgegrenzt
- [ ] Zoom/Pan aendert nur die Darstellung, nicht die Page-Groesse
- [ ] Bestehende Objekte und Operationen sind nicht beeintraechtigt

### Log

_(wird bei Umsetzung ausgefuellt)_

---

## Nicht in diesem Plan

- **Step F (Persistenz):** Wird in `story_ved_document_persistence_v1_0_1.md` integriert
- **Step 7 (Arbeitsvorlagen):** Eigene Story `story_document_templates_new_document.md`
- **Step 8 (PDF/Print):** Eigene Story `story_pdf_print_export.md`

---

## Betroffene Dateien (Zusammenfassung)

### Neue Dateien

| Datei | Modul | Inhalt |
|---|---|---|
| `src/ved_core/main/vec_units.h/cpp` | Core | DisplayUnit, UnitSettings, UnitFormatter |
| `src/ved_core/main/vec_measure_scale.h/cpp` | Core | MeasureScale, MeasureTick, ScaleCalculator |
| `src/ved_core/main/vec_document_settings.h/cpp` | Core | GridSettings, PageSettings, DocumentSettings, PageFormats |
| `tests/ved_core_units_tests.cpp` | Test | Unit-Konvertierung und Formatierung |
| `tests/ved_core_measure_scale_tests.cpp` | Test | Scale-Berechnung und Tick-Erzeugung |
| `tests/ved_core_document_settings_tests.cpp` | Test | PageFormats, DocumentSettings, Snapshot |

### Geaenderte Dateien

| Datei | Modul | Aenderung |
|---|---|---|
| `CMakeLists.txt` | Build | 6 neue Source-Dateien, 3 neue Test-Targets |
| `src/ved_core/main/vec_model.h/cpp` | Core | DocumentSettings Member, Getter/Setter, Snapshot |
| `src/ved_core/gengine/vec_graphic_engine.h` | Core | DrawRulers Signatur |
| `src/ved_qt/gengine/vec_graphic_engine_qt.cpp` | Qt | DrawRulers implementiert neue Signatur |
| `src/app/QVedWidget.h/cpp` | Qt | Grid/Ruler/Snap aus Model, drawPageBounds |
| `src/app/MainWindow.cpp` | Qt | Statusbar-Formatter, Workspace-Init |
