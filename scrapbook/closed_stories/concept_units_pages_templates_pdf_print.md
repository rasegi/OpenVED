# Konzept: Masseinheiten, Seiten, Arbeitsvorlagen, PDF und Print

Datum: 2026-05-20
Status: Konzept

## Ausgangslage

OpenVED arbeitet aktuell mit abstrakten Real-Koordinaten. Es gibt aber kein
explizites Dokumentkonzept fuer Masseinheiten wie `mm`, `cm` oder `inch`.

Grid und Lineal verwenden derzeit feste Werte in der Qt-Schicht, z.B.:

- Grid-Hauptabstand: `10000`
- Grid-Unterteilung: `10`

Die vorhandenen Werte deuten darauf hin, dass die historischen VED-Koordinaten
praktisch schon eine physische Bedeutung haben:

```text
1000 Real-Einheiten = 1 mm
10000 Real-Einheiten = 10 mm
A4 = 210000 x 297000 Real-Einheiten
```

Diese Annahme passt gut zu vorhandenen A4-nahen Arbeitsbereichen und soll als
Default verwendet werden. Die interne Geometrie muss dafuer nicht umgebaut
werden.

## Zielbild

VED soll Dokumente mit klar definierter physischer Groesse fuehren.

Ein neues Dokument kann aus einer Arbeitsvorlage entstehen, z.B.:

- A4 Portrait in Millimeter
- A4 Landscape in Millimeter
- A3 Portrait in Millimeter
- A3 Landscape in Millimeter
- Letter in Inch
- Custom

Jede Vorlage definiert:

- Ursprung bei `x=0`, `y=0`
- physische Seitengroesse
- Einheit und Umrechnungsfaktor
- Grid-Abstand und Unterteilungen
- optionale View-Defaults

Damit werden spaeter PDF-Export und Print eindeutig:

- Page-Groesse kommt aus dem Dokument.
- Model-Geometrie wird aus Real-Koordinaten in physische Einheiten umgerechnet.
- Zoom, Pan und View-State beeinflussen PDF/Print nicht.

## Grundsatzentscheidungen

### 1. Real-Koordinaten bleiben interne Geometrie

Objekte speichern weiterhin Real-Koordinaten:

- Linien
- Ellipsen
- Polygone
- Text
- Gruppen
- Kurven

Die Bedeutung der Real-Koordinate wird ueber Dokument-Settings beschrieben.

Default:

```text
realUnitsPerMillimeter = 1000.0
```

### 2. Einheit ist Core- und Dokumentkonzept

Einheiten duerfen nicht nur UI-Formatierung sein. Sie muessen im Core
modelliert und im Dokument gespeichert werden.

Qt darf Werte anzeigen und Eingaben annehmen, aber die fachliche Umrechnung
gehoert in den Core.

### 3. Eine Page pro Model fuer den MVP

Fuer den ersten Schritt gibt es genau eine Page pro `TDVecModel`.

Multi-Page, Artboards oder Layout-Sheets bleiben spaetere Erweiterungen.

### 4. Ursprung ist Page-Ursprung

Der Page-Ursprung ist:

```text
x = 0
y = 0
```

Fuer den MVP ist pragmatisch:

```text
0,0 = linke obere Ecke der Page
x positiv nach rechts
y positiv nach unten
```

Das passt zum aktuellen View-/Qt-Verhalten. PDF/Print kann die y-Achse intern
umrechnen, falls das Ausgabeformat einen anderen Ursprung verwendet.

### 5. Page ist Dokumentinhalt, View ist UI-Zustand

Im Dokument speichern:

- Einheit
- Real-zu-mm-Faktor
- Page-Format
- Page-Breite/-Hoehe
- Page-Orientierung
- Grid-Einstellungen

Nicht als Dokumentinhalt behandeln:

- aktueller Zoom
- aktueller Pan
- Grid sichtbar
- Lineal sichtbar
- Mouse-Crosshair sichtbar

Diese Werte gehoeren zum View-State. Sie koennen separat gespeichert werden,
sind aber nicht fachliche Dokumentgeometrie.

## Vorgeschlagene Core-Typen

### Display Unit

```cpp
enum class TDVecDisplayUnit {
    RawVed,
    Millimeter,
    Centimeter,
    Inch
};
```

`RawVed` ist hilfreich fuer Debugging und Legacy-Faelle.

### Unit Settings

```cpp
struct TDVecUnitSettings {
    double realUnitsPerMillimeter = 1000.0;
    TDVecDisplayUnit displayUnit = TDVecDisplayUnit::Millimeter;
};
```

Bedeutung:

- `realUnitsPerMillimeter` definiert die physische Skalierung des Dokuments.
- `displayUnit` ist die bevorzugte Anzeigeeinheit.

### Unit Formatter

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
};
```

Der Formatter wird fuer Statusbar, Lineal, Property-Editor und spaeter
Export-Dialoge verwendet.

### Grid Settings

```cpp
struct TDVecGridSettings {
    double majorStepReal = 10000.0;
    int subdivisions = 10;
    long resolutionLimitPixels = 1;
};
```

Default bedeutet:

```text
major = 10 mm
minor = 1 mm
```

Bei Inch-Vorlagen koennen andere Defaults gesetzt werden, z.B. 1 inch major mit
8 oder 16 subdivisions.

### Page Orientation

```cpp
enum class TDVecPageOrientation {
    Portrait,
    Landscape
};
```

### Page Settings

```cpp
struct TDVecPageSettings {
    std::string formatName = "A4";
    double widthReal = 210000.0;
    double heightReal = 297000.0;
    TDVecPageOrientation orientation = TDVecPageOrientation::Portrait;
};
```

Die Page beginnt bei `0,0`. Breite und Hoehe definieren den druckbaren bzw.
exportierbaren Dokumentrahmen.

### Document Settings

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

## Page-Formate und Vorlagen

Page-Formate sollten im Core als reine Daten/Factory-Funktionen existieren.

Beispiele:

```cpp
class TDVecPageFormats {
public:
    static TDVecPageSettings A4(const TDVecUnitSettings& units);
    static TDVecPageSettings A3(const TDVecUnitSettings& units);
    static TDVecPageSettings A5(const TDVecUnitSettings& units);
    static TDVecPageSettings Letter(const TDVecUnitSettings& units);
    static TDVecPageSettings Custom(
        std::string name,
        double widthReal,
        double heightReal,
        TDVecPageOrientation orientation);
};
```

Arbeitsvorlagen bauen darauf auf:

```cpp
struct TDVecDocumentTemplate {
    std::string id;
    std::string displayName;
    TDVecDocumentSettings documentSettings;
};
```

Beispiele:

```text
a4_mm_portrait
a4_mm_landscape
a3_mm_portrait
a3_mm_landscape
letter_in_portrait
custom
```

## Grid, Ruler und Statusbar

Grid, Lineal und Koordinatenanzeige muessen dieselbe Skala verwenden.

Heute berechnen Qt-Komponenten diese Werte teilweise selbst. Ziel:

- Core berechnet sinnvolle Steps und Labels.
- Qt zeichnet nur noch Punkte, Linien, Ticks und Texte.
- Statusbar nutzt denselben Formatter wie das Lineal.

Vorschlag:

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
```

Ein Core-Calculator entscheidet je nach Zoom/realPerPixel, welche Ticks
sichtbar und beschriftet werden.

## PDF und Print

PDF/Print darf nicht vom aktuellen View-Zustand abhaengen.

Massgeblich sind:

- `TDVecPageSettings`
- `TDVecUnitSettings`
- Model-Geometrie
- optional Druckränder und Export-Scale

Umrechnung:

```text
mm = real / realUnitsPerMillimeter
pdfPoints = mm / 25.4 * 72.0
```

Bei `0,0 = top-left` muss der PDF-Exporter y umrechnen:

```text
pdfY = pageHeightPoints - yPoints
```

Das ist Ausgabe-Logik und sollte nicht die interne Model-Koordinate veraendern.

## Persistenz

Das Dokumentformat muss langfristig speichern:

- `realUnitsPerMillimeter`
- `displayUnit`
- `grid.majorStepReal`
- `grid.subdivisions`
- `grid.resolutionLimitPixels`
- `page.formatName`
- `page.widthReal`
- `page.heightReal`
- `page.orientation`

Bei Dokumenten ohne diese Daten werden Defaults gesetzt:

```text
unit = Millimeter
realUnitsPerMillimeter = 1000.0
grid.majorStepReal = 10000.0
grid.subdivisions = 10
page = A4 Portrait
```

## Umsetzungsschritte

### Step 1: Core Units

- `vec_units.h/.cpp`
- `TDVecDisplayUnit`
- `TDVecUnitSettings`
- `TDVecUnitFormatter`
- Tests fuer mm/cm/inch-Konvertierung und Formatierung

### Step 2: Page Settings

- `TDVecPageOrientation`
- `TDVecPageSettings`
- `TDVecPageFormats`
- Tests fuer A4/A3/A5/Letter in Real-Einheiten

### Step 3: Document Settings im Model

- `TDVecDocumentSettings`
- Integration in `TDVecModel`
- Defaults so setzen, dass aktuelles Verhalten erhalten bleibt

### Step 4: Grid/Ruler auf Core-Skala umstellen

- `QVedWidget::drawGrid()` nutzt `TDVecGridSettings`
- `TDGraphicEngineQt::DrawRulers(...)` oder ein Nachfolger nutzt Core-Ticks
- Statusbar nutzt `TDVecUnitFormatter`

### Step 5: Page Bounds anzeigen

- Page-Rahmen bei `0,0,width,height` zeichnen
- Optionaler View-Toggle fuer Page Boundary
- Keine Objektbegrenzung erzwingen

### Step 6: Persistenz

- `TDVecDocumentSettings` in das VED-Dateiformat aufnehmen
- Defaults fuer alte/fehlende Settings
- Tests fuer Save/Load

### Step 7: Arbeitsvorlagen

- New-Document-Dialog oder Template-Auswahl
- Vorlagen erzeugen `TDVecDocumentSettings`
- Default: A4 Portrait mm

### Step 8: PDF/Print

- Export/Print verwendet PageSettings statt View-State
- Real-to-mm-to-points Mapping zentral testen
- Später optional Druckränder, Fit-to-page und Skalierung

## Risiken und Entscheidungen

### Historische Skalierung

Die Annahme `1000 Real = 1 mm` ist plausibel, sollte aber mit vorhandenen
Dokumenten/Erwartungen validiert werden.

### Display Unit dokumentbezogen oder Benutzerpräferenz

Empfehlung fuer MVP:

- im Dokument speichern
- spaeter optional User-Override fuer Anzeige

### Grid dokumentbezogen oder View-State

Empfehlung:

- Grid-Abstand und Unterteilung sind dokumentbezogen
- Sichtbarkeit und Snap aktiv/inaktiv sind View-State

### Eine oder mehrere Pages

Empfehlung:

- MVP: eine Page pro Model
- spaeter Multi-Page/Artboards als eigene Story

### Ursprung oben links oder unten links

Empfehlung:

- MVP: oben links, weil es zum aktuellen Screen-Verhalten passt
- Exporter konvertiert bei Bedarf

## Akzeptanzkriterien

- Ein neues A4-Dokument hat Page-Bounds `0,0` bis `210000,297000`.
- `TDVecUnitFormatter` formatiert dieselbe Koordinate konsistent fuer mm, cm
  und inch.
- Grid, Lineal und Statusbar verwenden dieselbe Unit-Logik.
- Bestehende Dokumente ohne Settings bekommen A4/mm-Defaults.
- PDF/Print kann aus PageSettings eine physisch korrekte Zielgroesse ableiten.
- Qt enthaelt keine fachliche Umrechnung zwischen Real, mm, cm und inch.

## Bezug zu bestehenden Stories

- `story_core_units_grid_rulers_page_settings.md`
  - diese Konzeptnotiz konkretisiert Zielbild und Reihenfolge

- `story_ved_document_persistence_v1_0_1.md`
  - DocumentSettings muessen langfristig Teil der Persistence werden

- `story_operation_helper_line_status_metrics.md`
  - Operation-Metriken sollten spaeter denselben UnitFormatter verwenden
