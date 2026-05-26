# Story: Core Units, Grid/Ruler Scale und Page Settings

Datum: 2026-05-15

## Kontext

Im Qt-Port wurden in Step 2.016 und Step 2.017 neue View-Hilfen eingefuehrt:

- optionale Mouse-Tolerance-Crosshair-Anzeige
- Koordinatenanzeige in der Statusbar
- horizontales und vertikales Lineal

Dabei wurde sichtbar, dass Grid und Lineal aktuell zwar dieselbe visuelle Skala verwenden, die eigentliche Einheit/Formatierung aber noch nicht als Kernkonzept modelliert ist.

Die aktuellen Konstanten deuten stark darauf hin, dass VED intern Real-Koordinaten in einer feinen Laengeneinheit fuehrt:

- Workspace A4-nahe Breite: `210000`
- Workspace A4-nahe Hoehe: `296985`
- Grid-Hauptabstand: `10000`

Das passt fachlich gut zu:

- `1000` Real-Einheiten = `1 mm`
- `10000` Real-Einheiten = `10 mm`
- A4 = ca. `210000 x 297000`

Diese Annahme muss spaeter validiert werden, soll aber als naheliegender Default dienen.

## Ziel

VED soll ein Qt-unabhaengiges Core-Konzept fuer Masseinheiten, Grid/Ruler-Skala und Papier-/Dokumentformat erhalten.

Wichtig:

- Das ist Kern von VED, nicht Qt.
- Qt soll nur anzeigen und Eingaben weiterreichen.
- Grid, Lineal, Koordinatenanzeige, Export, Print und spaeter Page-Bounds sollen dieselbe Core-Skala verwenden.

## Nicht-Ziele

- Keine direkte Abhaengigkeit von Qt-Typen wie `QString`, `QColor`, `QPainter`, `QRect`.
- Kein UI-Dialog in dieser Story.
- Keine sofortige vollstaendige Save/Load-Migration, falls das Dateiformat noch nicht stabil ist.
- Kein Umbau aller Operationen auf einmal.

## Vorgeschlagene Core-Architektur

### 1. Display Unit

Neue Core-Dateien, z.B.:

- `src/ved_core/main/vec_units.h`
- `src/ved_core/main/vec_units.cpp`

Vorschlag:

```cpp
enum class TDVecDisplayUnit {
    RawVed,
    Micrometer,
    Millimeter,
    Centimeter,
    Inch
};

struct TDVecUnitSettings {
    double realUnitsPerMillimeter = 1000.0;
    TDVecDisplayUnit displayUnit = TDVecDisplayUnit::Millimeter;
};
```

Bedeutung:

- `realUnitsPerMillimeter` beschreibt die interne Dokument-Skalierung.
- `displayUnit` beschreibt nur die bevorzugte Anzeige.
- Die Geometrie im Model bleibt in Real-Koordinaten.

### 2. Unit Formatter

Der Formatter soll Labels fuer Lineal, Grid und Statusbar liefern.

```cpp
class TDVecUnitFormatter {
public:
    explicit TDVecUnitFormatter(TDVecUnitSettings settings);

    double ToMillimeters(double realValue) const;
    double FromMillimeters(double millimeters) const;
    double ToDisplayUnit(double realValue) const;
    double FromDisplayUnit(double displayValue) const;

    std::string FormatCoordinate(double realValue) const;
    std::string FormatLength(double realLength) const;

    const TDVecUnitSettings& Settings() const;

private:
    TDVecUnitSettings settings_;
};
```

Der Formatter ist Core-Code und nutzt `std::string`, nicht `QString`.

### 3. Measure Scale fuer Grid und Ruler

Grid und Lineal sollen nicht getrennt ihre Tick-Schritte berechnen. Stattdessen soll der Core eine gemeinsame Skala liefern.

```cpp
struct TDVecMeasureTick {
    double realValue = 0.0;
    bool major = false;
    bool labeled = false;
    std::string label;
};

struct TDVecMeasureScale {
    double minorStepReal = 1000.0;
    double majorStepReal = 10000.0;
    double labelStepReal = 10000.0;
};

class TDVecMeasureScaleCalculator {
public:
    TDVecMeasureScaleCalculator(TDVecUnitSettings unitSettings);

    TDVecMeasureScale CalculateForView(
        double realPerPixel,
        long minimumPixelDistance,
        double preferredMajorStepReal,
        int subdivisions) const;

    std::vector<TDVecMeasureTick> BuildTicks(
        double visibleMinReal,
        double visibleMaxReal,
        const TDVecMeasureScale& scale) const;

private:
    TDVecUnitFormatter formatter_;
};
```

Ziel:

- Grid verwendet `minorStepReal` und `majorStepReal`.
- Ruler verwendet dieselben Steps plus Labels.
- Statusbar kann denselben Formatter nutzen.
- Zoom/Pan beeinflusst nur die sichtbare Range und `realPerPixel`, nicht die Semantik der Einheit.

### 4. Grid Settings im Dokument

Grid-Einstellungen sollen dokumentbezogen sein, nicht nur UI-Zustand.

```cpp
struct TDVecGridSettings {
    double majorStepReal = 10000.0;
    int subdivisions = 10;
    long resolutionLimitPixels = 1;
};
```

Unterscheidung:

- `TDVecGridSettings` gehoert perspektivisch zum Dokument/Model.
- `showGrid` gehoert zur View/UI und wird nicht zwingend im Dokument gespeichert.

### 5. Page Settings im Dokument

Papierformat ist Dokumentinhalt bzw. Dokumentrahmen und sollte im Model gespeichert werden.

```cpp
enum class TDVecPageOrientation {
    Portrait,
    Landscape
};

struct TDVecPageSettings {
    std::string formatName = "A4";
    double widthReal = 210000.0;
    double heightReal = 297000.0;
    TDVecPageOrientation orientation = TDVecPageOrientation::Portrait;
};
```

Spaeter koennen vordefinierte Formate ergaenzt werden:

```cpp
class TDVecPageFormats {
public:
    static TDVecPageSettings A4(const TDVecUnitSettings& units);
    static TDVecPageSettings A3(const TDVecUnitSettings& units);
    static TDVecPageSettings A5(const TDVecUnitSettings& units);
    static TDVecPageSettings Custom(
        std::string name,
        double widthReal,
        double heightReal,
        TDVecPageOrientation orientation);
};
```

### 6. Document Settings im Model

Der Model-Kern sollte diese Settings gesammelt fuehren.

```cpp
struct TDVecDocumentSettings {
    TDVecUnitSettings unitSettings;
    TDVecGridSettings gridSettings;
    TDVecPageSettings pageSettings;
};
```

Integration in `TDVecModel`:

```cpp
class TDVecModel {
public:
    const TDVecDocumentSettings& DocumentSettings() const;
    void SetDocumentSettings(const TDVecDocumentSettings& settings);

    const TDVecUnitSettings& UnitSettings() const;
    const TDVecGridSettings& GridSettings() const;
    const TDVecPageSettings& PageSettings() const;

private:
    TDVecDocumentSettings documentSettings_;
};
```

## Was wird gespeichert?

Langfristig im Model/Dokument speichern:

- `realUnitsPerMillimeter`
- `displayUnit`
- `grid.majorStepReal`
- `grid.subdivisions`
- `page.formatName`
- `page.widthReal`
- `page.heightReal`
- `page.orientation`

Nicht zwingend im Model/Dokument speichern:

- Grid sichtbar/an/aus
- Ruler sichtbar/an/aus
- Mouse-Crosshair sichtbar/an/aus
- aktueller Zoom
- aktueller Pan/Scroll

Diese Werte sind View-/UI-Zustand.

## Auswirkungen auf bestehende Qt-Implementierung

Heute:

- `QVedWidget` haelt Konstanten:
  - `kGridDistance`
  - `kGridSubDivisions`
  - `kGridResolutionLimit`
- `TDGraphicEngineQt::DrawRulers(...)` berechnet Tick/Label-Schritte lokal.

Nach Umsetzung:

- `QVedWidget` fragt die Grid-/Unit-Settings aus `TDVecModel` oder `TDVecEditCad`.
- Grid und Ruler erhalten eine Core-generierte `TDVecMeasureScale`.
- Qt zeichnet nur:
  - Linien
  - Ticks
  - Labels
- Qt formatiert nicht fachlich selbst, sondern zeigt die Core-Labels.

## Vorgeschlagene Umsetzungsschritte

### Step A: Core-Datentypen einfuehren

- `vec_units.h/.cpp`
- `TDVecDisplayUnit`
- `TDVecUnitSettings`
- `TDVecUnitFormatter`
- Unit-Tests fuer Umrechnung und Label-Formatierung.

### Step B: Grid/Ruler-Scale in Core verlagern

- `TDVecMeasureScale`
- `TDVecMeasureTick`
- `TDVecMeasureScaleCalculator`
- Tests fuer Zoom-nahe `realPerPixel`-Faelle.

### Step C: TDVecModel DocumentSettings

- `TDVecDocumentSettings` in Model integrieren.
- Defaults so setzen, dass aktuelles Verhalten erhalten bleibt:
  - `realUnitsPerMillimeter = 1000.0`
  - `displayUnit = Millimeter`
  - `majorStepReal = 10000.0`
  - `subdivisions = 10`
  - `page = A4`

### Step D: QVedWidget und TDGraphicEngineQt anbinden

- `QVedWidget::drawGrid()` nutzt Model/GridSettings.
- `QVedWidget::drawRulers()` nutzt Core-Scale.
- `TDGraphicEngineQt` zeichnet Ticks/Labels anhand vorbereiteter Core-Daten.

### Step E: Page Bounds anzeigen

- Optionaler View-Toggle fuer Papiergrenze.
- GraphicEngine zeichnet Page-Bounds aus `TDVecPageSettings`.
- Spaeter relevant fuer Print/PDF/SVG/DXF-Export.

### Step F: Persistenz

- Erst wenn Save/Load stabil genug ist:
  - `TDVecDocumentSettings` ins Dokumentformat schreiben.
  - Bei alten Dokumenten Defaults setzen.

## Offene Fragen

- Ist die Annahme korrekt, dass VED-Real-Einheiten historisch Mikrometer sind?
- Soll `displayUnit` dokumentbezogen oder user-/viewbezogen sein?
- Soll Grid-Abstand dokumentbezogen sein oder nur View-Zustand?
- Soll Page-Format immer einen sichtbaren Arbeitsbereich definieren oder nur optionalen Druckrahmen?
- Soll es mehrere Pages/Artboards geben oder genau eine Page pro Model?

## Akzeptanzkriterien

- Core enthaelt alle Mass-/Skalenentscheidungen ohne Qt-Abhaengigkeit.
- Qt enthaelt keine fachliche Unit-Umrechnung mehr.
- Grid und Ruler zeigen konsistente Ticks.
- Statusbar-Koordinaten nutzen denselben Formatter.
- Defaults erhalten das aktuelle sichtbare Verhalten.
- Tests decken Umrechnung, Formatierung und Tick-Erzeugung ab.
