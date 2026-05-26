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

- [x] PageFormats A4 Portrait: `210000 x 297000`
- [x] PageFormats A4 Landscape: `297000 x 210000`
- [x] PageFormats A3 Portrait: `297000 x 420000`
- [x] PageFormats A3 Landscape: `420000 x 297000`
- [x] PageFormats A5 Portrait: `148000 x 210000`
- [x] PageFormats Letter Portrait: `215900 x 279400` (8.5 x 11 in)
- [x] PageFormats Letter Landscape: `279400 x 215900`
- [x] Custom: beliebige Werte
- [x] DocumentSettings Defaults = A4/mm/10000-10
- [x] TDVecModel Default-DocumentSettings korrekt
- [x] Convenience-Getter (UnitSettings, GridSettings, PageSettings)
- [x] SetDocumentSettings + Getter-Roundtrip
- [x] SetDocumentSettings markiert Model als changed
- [x] Snapshot: DocumentSettings wird mitgesichert
- [x] Snapshot: RestoreSnapshot stellt DocumentSettings wieder her

### Log

- 2026-05-26: `vec_document_settings.h/cpp` erstellt, `TDVecModel` erweitert
- 15 Tests in `ved_core_document_settings_tests.cpp`, alle bestanden
- `bottomRightArea` von 296985 auf 297000 korrigiert (exaktes A4)
- Snapshot um DocumentSettings erweitert (CreateSnapshot + RestoreSnapshot)
- Gesamte Test-Suite: 24/24 bestanden, App baut sauber

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
- [ ] Ruler zeigt dieselben Ticks und Labels (jetzt mit mm-Suffix)
- [ ] Statusbar zeigt Koordinaten mit Einheit
- [ ] Grid-Snap funktioniert wie vorher
- [ ] Verschiedene Zoom-Stufen: Grid-Aufloesung passt sich an
- [x] Bestehende `ctest`-Suite laeuft weiterhin durch (24/24)

### Log

- 2026-05-26: Qt-Anbindung umgesetzt
- `DrawRulers` Signatur geaendert: `(long,long,long)` → `(TDVecMeasureScale&, TDVecUnitFormatter&)`
- `niceGridStepAtLeast` aus QVedWidget.cpp entfernt → nutzt `NiceStepAtLeast` aus Core
- `niceRulerStepAtLeast` + `isMainRulerValue` aus vec_graphic_engine_qt.cpp entfernt
- Grid/Ruler/Snap lesen Settings aus DocumentSettings (mit Fallback auf Defaults)
- Statusbar nutzt `TDVecUnitFormatter` fuer formatierte Koordinaten
- Workspace-Groesse aus PageSettings statt Hardcoded-Konstanten
- QVedWidget erhaelt DocumentSettings-Pointer via `setDocumentSettings()`
- Build + 24/24 Tests gruen, App baut sauber
- Visuelle Verifikation steht noch aus

---

## Step E: Page Bounds anzeigen

### Konzept: Papier als visuelle Schablone

Das Papier ist **nicht** der Zeichenbereich. Der Zeichenbereich (Workspace) ist unbegrenzt —
man kann beliebig grosse Objekte zeichnen, frei zoomen und scrollen. Das Papier ist eine
**visuelle Schablone**, die zeigt welcher Ausschnitt spaeter gedruckt wird.

Die Position der Schablone im Koordinatensystem wird durch `pageOriginX` / `pageOriginY`
in `TDVecPageSettings` bestimmt (default: `0.0, 0.0`). Damit kann man das Papier-Rechteck
frei im Koordinatensystem verschieben, z.B. um einen bestimmten Teil der Zeichnung
als Druckausschnitt zu waehlen.

### Was wird gemacht

**`vec_document_settings.h` (Core):**
- `TDVecPageSettings` erhaelt zwei neue Felder: `pageOriginX = 0.0`, `pageOriginY = 0.0`

**`QVedWidget.h/cpp` (Qt):**
- Neues Member `bool showPageBounds_ = true`
- Neue Methode `drawPageBounds()`:
  - Page-Bereich (`{originX, originY}` bis `{originX+width, originY+height}`) bleibt weiss
  - Bereich ausserhalb der Page wird hellgrau gefuellt (Overlay mit `QColor(200, 200, 200, 120)`)
  - Duenne Linie am Page-Rand (1px, dunkelgrau)
- Umsetzung: 4 Rechtecke (links, rechts, oben, unten) um die Page herum fuellen
- Aufruf in `paintEvent` vor `drawGrid()`
- `showPageBounds_` ist View-State — nicht im Dokument gespeichert

### Tests

**Core-Tests** (in `ved_core_document_settings_tests.cpp` ergaenzen):

- [ ] PageSettings Default: `pageOriginX == 0.0`, `pageOriginY == 0.0`
- [ ] PageFormats (A4, A3 etc.) haben Origin `0.0, 0.0`
- [ ] Snapshot: pageOriginX/Y werden mitgesichert und wiederhergestellt

**Visuell:**

- [ ] Page-Bereich ist weiss, Bereich ausserhalb ist hellgrau
- [ ] Page-Rand als duenne Linie sichtbar
- [ ] Zoom/Pan aendert nur die Darstellung, nicht die Page-Groesse
- [ ] Grid und Objekte werden ueber der Page korrekt gezeichnet
- [ ] Bestehende Operationen sind nicht beeintraechtigt

### Log

- 2026-05-26: `pageOriginX/Y` in `TDVecPageSettings` eingefuegt (Core)
- `QPainter* Painter()` Getter in `TDGraphicEngineQt` ergaenzt
- `drawPageBounds()` in QVedWidget: weisses Page-Rechteck + grauer Hintergrund + Rahmen
- `initializeViewIfNeeded()` nutzt `pageOriginX/Y` fuer Workspace/World/View-Range
- `MainWindow`: `initializeEditor()` und `newDocument()` nutzen `pageOriginX/Y`
- 2 neue Tests (Origin-Defaults, PageFormats-Origin), Snapshot-Tests um Origin erweitert
- Canvas-Hintergrund von fast-weiss auf grau geaendert (einheitlich mit Widget-Hintergrund)
- Build + 24/24 Tests gruen
- Visuelle Verifikation steht noch aus

---

## Step F: Papierformat in Fusszeile

### Was wird gemacht

**`MainWindow.h/cpp` (Qt):**
- Neues Member `QLabel* pageFormatLabel_`
- In Konstruktor: Label in Statusbar einfuegen (zwischen Operation und Koordinaten)
- Neue Methode `updatePageFormatStatus()`:
  - Zeigt z.B. `"A4 Portrait — 210.00 × 297.00 mm"`
  - Nutzt `TDVecUnitFormatter` fuer die Dimensionen in der aktuellen Display-Einheit
  - Format: `"{formatName} {Orientation} — {breite} × {hoehe} {einheit}"`
- Aufruf bei:
  - `initializeEditor()` (initialer Zustand)
  - `installModel()` (Dokument geladen)
  - `newDocument()` (neues Dokument)
  - Spaeter: nach Papierformat-Aenderung (Step G)

### Tests

Kein eigenes Test-Target — Verifikation visuell:

- [ ] Fusszeile zeigt Papierformat + Dimensionen mit Einheit
- [ ] Bei neuem Dokument: `"A4 Portrait — 210.00 × 297.00 mm"`
- [ ] Label ist lesbar positioniert zwischen Operation und Koordinaten

### Log

_(wird bei Umsetzung ausgefuellt)_

---

## Step G: Papierformat-Wechsel Dialog

### Was wird gemacht

**Neue Datei `src/app/PageSetupDialog.h/cpp` (Qt):**
- `class PageSetupDialog : public QDialog`

**Dialog-Layout (von oben nach unten):**

```
+------------------------------------------+
|  Page Setup                              |
+------------------------------------------+
|                                          |
|  Format:      [  A4              v ]     |
|  Orientation: [  Portrait        v ]     |
|                                          |
|  --- Abmessungen ---                     |
|  Breite:      [ 210.00     ] mm          |
|  Hoehe:       [ 297.00     ] mm          |
|                                          |
|  --- Position im Zeichenbereich ---      |
|  Origin X:    [   0.00     ] mm          |
|  Origin Y:    [   0.00     ] mm          |
|                                          |
|            [ OK ]  [ Abbrechen ]         |
+------------------------------------------+
```

**UI-Elemente im Detail:**

| Element | Typ | Verhalten |
|---|---|---|
| Format | `QComboBox` | A4, A3, A5, Letter, Custom. Bei Wechsel: Breite/Hoehe aus `TDVecPageFormats` setzen. Bei Custom: SpinBoxes werden editierbar |
| Orientation | `QComboBox` | Portrait, Landscape. Bei Wechsel: Breite und Hoehe tauschen |
| Breite | `QDoubleSpinBox` | Read-only bei Standard-Formaten, editierbar bei Custom. Bereich: 1.0 – 5000.0 mm (bzw. aequivalent in aktueller Einheit). 2 Dezimalstellen |
| Hoehe | `QDoubleSpinBox` | Wie Breite |
| Origin X | `QDoubleSpinBox` | Immer editierbar. Position der Papier-Schablone im Koordinatensystem. Bereich: -10000.0 – 10000.0 mm. Default 0.0 |
| Origin Y | `QDoubleSpinBox` | Wie Origin X |
| Einheit-Label | `QLabel` | Zeigt aktuelle DisplayUnit (mm/cm/in) neben jedem SpinBox |
| OK / Abbrechen | `QDialogButtonBox` | Standard-Buttons |

**Einheiten-Logik:**
- SpinBoxes zeigen Werte in der aktuellen `DisplayUnit` (mm, cm, in)
- Intern wird alles in Real-Einheiten gespeichert
- Dialog erhaelt `TDVecUnitSettings` im Konstruktor fuer Umrechnung
- Bei Anzeige: `TDVecUnitFormatter::ToDisplayUnit(realValue)` → SpinBox
- Bei Uebernahme: `TDVecUnitFormatter::FromDisplayUnit(displayValue)` → Real

**Interaktionslogik:**
- Format-Wechsel zu Standard (A4/A3/A5/Letter):
  - Breite/Hoehe werden aus `TDVecPageFormats` geladen und read-only gesetzt
  - Orientation wird beibehalten, Breite/Hoehe entsprechend getauscht
- Format-Wechsel zu Custom:
  - Breite/Hoehe bleiben auf den aktuellen Werten, werden editierbar
- Orientation-Wechsel:
  - Breite und Hoehe werden getauscht (swap)
  - Bei Standard-Format: bleibt konsistent mit dem Format
- Origin-Aenderung:
  - Verschiebt die Papier-Schablone im Koordinatensystem
  - Unabhaengig von Format und Orientation

**Rueckgabe:** `TDVecPageSettings` mit allen gewaehlten Werten (Format, Breite, Hoehe, Orientation, OriginX, OriginY)

---

**`MainWindow.h/cpp` (Qt):**
- Neuer Menue-Eintrag: `Format > Page Setup...` (mit Shortcut, z.B. keiner oder Ctrl+Shift+P)
- Slot `onPageSetup()`:
  1. Snapshot erstellen (fuer Undo)
  2. `PageSetupDialog` oeffnen mit aktuellen `DocumentSettings`
  3. Bei OK:
     - Neue `TDVecDocumentSettings` zusammenbauen (PageSettings aus Dialog, Rest beibehalten)
     - `model_->SetDocumentSettings(newSettings)`
     - `model_->SetTopLeftArea({originX, originY})`
     - `model_->SetBottomRightArea({originX + width, originY + height})`
     - `canvas_->resetView()` (View an neue Page anpassen)
     - `updatePageFormatStatus()` (Step F)
  4. Bei Abbrechen: Snapshot verwerfen

**Undo-Mechanismus:**
- Vor Dialog-Oeffnung: `TDVecModelSnapshot` erstellen
- Bei OK: Snapshot wird in die Undo-History geschoben (ueber `operationManager_`)
- Bei Undo: `RestoreSnapshot` stellt vorherige DocumentSettings + TopLeft/BottomRight wieder her
- Hinweis: Falls der bestehende Undo-Mechanismus keinen generischen Snapshot-Push unterstuetzt,
  wird ein einfacher Ansatz gewaehlt (z.B. Model als changed markieren ohne Undo-Eintrag)
  und vollstaendiger Undo-Support in einer separaten Story nachgezogen

**`CMakeLists.txt`:**
- `PageSetupDialog.h/cpp` in `add_executable` einfuegen

### Tests

Kein eigenes Test-Target — Verifikation visuell:

- [ ] Dialog oeffnet sich ueber Menue `Format > Page Setup...`
- [ ] Aktuelles Format/Orientation/Groesse/Origin sind vorausgewaehlt
- [ ] Format-Wechsel (z.B. A4 → A3) aktualisiert Breite/Hoehe korrekt
- [ ] Orientation-Wechsel tauscht Breite/Hoehe
- [ ] Custom erlaubt freie Eingabe von Breite/Hoehe
- [ ] Origin X/Y sind editierbar und aendern die Papier-Position
- [ ] Einheiten-Labels zeigen aktuelle DisplayUnit
- [ ] OK uebernimmt Aenderung:
  - [ ] Page Bounds (Step E) verschieben sich bei Origin-Aenderung
  - [ ] Page Bounds aendern Groesse bei Format-Wechsel
  - [ ] Fusszeile (Step F) aktualisiert sich
  - [ ] View zentriert sich auf neue Page
- [ ] Abbrechen verwirft alle Aenderungen
- [ ] Erneutes Oeffnen zeigt die zuletzt gesetzten Werte

### Log

_(wird bei Umsetzung ausgefuellt)_

---

## Nicht in diesem Plan

- **Persistenz:** Wird in `story_ved_document_persistence_v1_0_1.md` integriert
- **Arbeitsvorlagen:** Eigene Story `story_document_templates_new_document.md`
- **PDF/Print:** Eigene Story `story_pdf_print_export.md`
- **Druckraender (Print Margins):** Gestrichelte Linie innerhalb der Page-Schablone, die den bedruckbaren Bereich zeigt — gehoert zur PDF/Print-Story

---

## Betroffene Dateien (Zusammenfassung)

### Neue Dateien

| Datei | Modul | Inhalt |
|---|---|---|
| `src/ved_core/main/vec_units.h/cpp` | Core | DisplayUnit, UnitSettings, UnitFormatter |
| `src/ved_core/main/vec_measure_scale.h/cpp` | Core | MeasureScale, MeasureTick, ScaleCalculator |
| `src/ved_core/main/vec_document_settings.h/cpp` | Core | GridSettings, PageSettings, DocumentSettings, PageFormats |
| `src/app/PageSetupDialog.h/cpp` | Qt | Dialog fuer Papierformat-Wechsel |
| `tests/ved_core_units_tests.cpp` | Test | Unit-Konvertierung und Formatierung |
| `tests/ved_core_measure_scale_tests.cpp` | Test | Scale-Berechnung und Tick-Erzeugung |
| `tests/ved_core_document_settings_tests.cpp` | Test | PageFormats, DocumentSettings, Snapshot |

### Geaenderte Dateien

| Datei | Modul | Aenderung |
|---|---|---|
| `CMakeLists.txt` | Build | 6+2 Source-Dateien, 3 Test-Targets |
| `src/ved_core/main/vec_model.h/cpp` | Core | DocumentSettings Member, Getter/Setter, Snapshot |
| `src/ved_core/gengine/vec_graphic_engine.h` | Core | DrawRulers Signatur |
| `src/ved_qt/gengine/vec_graphic_engine_qt.cpp` | Qt | DrawRulers implementiert neue Signatur |
| `src/app/QVedWidget.h/cpp` | Qt | Grid/Ruler/Snap aus Model, drawPageBounds |
| `src/app/MainWindow.h/cpp` | Qt | Statusbar (Koordinaten + Papierformat), Page-Setup-Menue |
