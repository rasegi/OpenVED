# Story: PDF-Export und Print

Datum: 2026-05-26
Status: offen

## Kontext

Das Konzept `concept_units_pages_templates_pdf_print.md` definiert in Step 8
den PDF-Export und Print-Pfad fuer VED.

Voraussetzung ist, dass folgende Core-Typen existieren und im Model integriert
sind (aus `story_core_units_grid_rulers_page_settings.md`):

- `TDVecPageSettings` (Seitengroesse, Orientierung)
- `TDVecUnitSettings` (`realUnitsPerMillimeter`)
- `TDVecDocumentSettings`

Aktuell hat VED keinen Export- oder Druckpfad. Die interne Geometrie liegt in
Real-Koordinaten vor und muss fuer die Ausgabe in physische Einheiten
umgerechnet werden.

## Ziel

VED soll Dokumente als PDF exportieren und ueber den System-Druckdialog
drucken koennen. Die Ausgabe basiert ausschliesslich auf den
Dokument-Settings — nicht auf dem aktuellen View-Zustand (Zoom, Pan).

## Nicht-Ziele

- Kein SVG-, DXF- oder Raster-Export (spaetere Stories).
- Keine Multi-Page-Dokumente (MVP: eine Page pro Model).
- Kein Druckvorschau-Dialog mit eigenem Rendering.
- Keine Seitenumbruch-Logik.

## Grundsatzentscheidungen

### Koordinaten-Mapping

Die zentrale Umrechnung von Model-Geometrie zu PDF/Print-Koordinaten:

```text
mm = real / realUnitsPerMillimeter
points = mm / 25.4 * 72.0
```

Bei `0,0 = top-left` im Model muss y fuer PDF umgerechnet werden:

```text
pdfY = pageHeightPoints - yPoints
```

Diese Umrechnung ist reine Ausgabe-Logik und veraendert keine
Model-Koordinaten.

### View-Unabhaengigkeit

Massgeblich fuer die Ausgabe sind:

- `TDVecPageSettings` (Seitengroesse)
- `TDVecUnitSettings` (Skalierungsfaktor)
- Model-Geometrie (Objekte)
- optional: Druckraender und Export-Skalierung

Nicht massgeblich:

- aktueller Zoom
- aktueller Pan
- Grid-/Ruler-Sichtbarkeit

## Vorgeschlagene Core-Architektur

### Export Coordinate Mapper

```cpp
class TDVecExportCoordinateMapper {
public:
    explicit TDVecExportCoordinateMapper(
        const TDVecUnitSettings& unitSettings,
        const TDVecPageSettings& pageSettings);

    double RealToMillimeters(double realValue) const;
    double MillimetersToPoints(double mm) const;
    double RealToPoints(double realValue) const;

    double MapX(double realX) const;
    double MapY(double realY) const;

    double PageWidthPoints() const;
    double PageHeightPoints() const;

private:
    TDVecUnitSettings unitSettings_;
    TDVecPageSettings pageSettings_;
};
```

### Export Settings

```cpp
struct TDVecExportSettings {
    double marginTopMm = 0.0;
    double marginBottomMm = 0.0;
    double marginLeftMm = 0.0;
    double marginRightMm = 0.0;
    double scaleFactor = 1.0;
    bool fitToPage = false;
};
```

Fuer den MVP sind alle Raender `0` und `scaleFactor = 1.0`.

## Qt-Integration

### PDF-Export

- Menue `File > Export PDF...`
- Datei-Dialog fuer Zielpfad
- `QPdfWriter` oder `QPrinter` im PDF-Modus
- Page-Groesse aus `TDVecPageSettings`
- Objekte ueber `TDVecExportCoordinateMapper` auf PDF-Koordinaten abbilden
- Rendering ueber bestehende GraphicEngine oder einen dedizierten Export-Renderer

### Print

- Menue `File > Print...`
- System-Druckdialog (`QPrintDialog`)
- Seitengroesse aus `TDVecPageSettings` als Vorschlag
- Dasselbe Koordinaten-Mapping wie PDF-Export

### Rendering-Strategie

Zwei Optionen:

**Option A — GraphicEngine wiederverwenden:**
Eine `TDGraphicEngineQt`-Instanz mit `QPainter` auf `QPdfWriter`/`QPrinter`
konfigurieren. View-Transformation so setzen, dass die gesamte Page 1:1
abgebildet wird.

**Option B — Dedizierter Export-Renderer:**
Ein separater Renderer iteriert ueber Model-Objekte und zeichnet sie direkt
mit `QPainter` unter Verwendung des `TDVecExportCoordinateMapper`.

Empfehlung fuer MVP: Option A, da die bestehende GraphicEngine das
Objekt-Rendering bereits beherrscht.

## Vorgeschlagene Umsetzungsschritte

### Step 1: Core Coordinate Mapper

- `TDVecExportCoordinateMapper`
- `TDVecExportSettings`
- Tests: Real-to-mm-to-points Konvertierung fuer A4, A3, Letter
- Tests: y-Achsen-Umrechnung korrekt

### Step 2: PDF-Export ueber Qt

- `File > Export PDF...` in MainWindow
- `QPdfWriter` mit Page-Groesse aus DocumentSettings
- GraphicEngine-Rendering auf PDF-QPainter
- Test: exportiertes PDF hat korrekte Seitengroesse

### Step 3: Print ueber Qt

- `File > Print...` in MainWindow
- `QPrintDialog` mit Seitengroesse-Vorschlag
- Dasselbe Rendering wie PDF-Export
- Test: Druckausgabe hat korrekte Skalierung

### Step 4: Export Settings (optional, nach MVP)

- Druckraender konfigurierbar
- Fit-to-page Option
- Skalierungsfaktor
- Export-Dialog mit Vorschau der Einstellungen

## Abhaengigkeiten

- `story_core_units_grid_rulers_page_settings.md`:
  - **Step A** (Core-Datentypen: `TDVecDisplayUnit`, `TDVecUnitSettings`,
    `TDVecUnitFormatter`) — **offen**
  - **Step C** (DocumentSettings im Model: `TDVecDocumentSettings`,
    `TDVecPageSettings`, `TDVecGridSettings`) — **offen**
- `concept_units_pages_templates_pdf_print.md` — Konzept Step 8

## Akzeptanzkriterien

- PDF-Export erzeugt eine Datei mit physisch korrekter Seitengroesse.
- Real-Koordinaten werden korrekt in PDF-Points umgerechnet.
- Die Ausgabe ist unabhaengig vom aktuellen View-Zustand.
- Objekte erscheinen in korrekter Position und Groesse auf der PDF-Seite.
- Print verwendet denselben Rendering-Pfad wie PDF-Export.
- Bestehende Model-Geometrie bleibt unveraendert.
