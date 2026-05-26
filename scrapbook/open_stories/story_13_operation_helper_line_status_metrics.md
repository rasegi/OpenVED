ok# Story: Winkel und Abstand aktiver Hilfslinien in der Statusleiste

## Ziel

Bei Operationen, die nach dem ersten Punkt eine dynamische Hilfslinie zum aktuellen Mauspunkt zeichnen, sollen in der Fusszeile neben `X` und `Y` auch angezeigt werden:

- Winkel der Hilfslinie
- Abstand/Laenge der Hilfslinie in nativen VED-Koordinaten

Die Werte sollen sich waehrend `MouseMove` laufend aktualisieren.

## Motivation

Viele Create-Operationen arbeiten nach demselben Muster:

1. Benutzer setzt den ersten Punkt.
2. Beim Bewegen der Maus entsteht eine temporaere Hilfslinie.
3. Diese Hilfslinie definiert Richtung und Abstand bis zum aktuellen Cursor.
4. Der zweite Klick uebernimmt diese aktuelle Geometrie.

Aktuell sieht der Benutzer die Linie visuell, aber nicht numerisch. Fuer praezises Zeichnen sind Winkel und Abstand wichtig.

## Betroffene Operationen

Direkt relevant:

- `VOC_LINE`
- `VOC_ELLIPSE_MIDPOINT`
- `VOC_POLYGON_SMARTLINE`
- `VOC_POLYCURVE`
- `VOC_RECTANGLE_ROTATED`

Sobald portiert ebenfalls relevant:

- `VOC_BEZIERCURVE_CONTROLPOINT`
- `VOC_BSPLINE_CONTROLPOINT`

Weitere Operationen koennen spaeter teilnehmen, wenn sie eine eindeutige aktive Hilfslinie besitzen.

## Definition der Werte

### X/Y

- Bleiben die aktuellen realen VED-Koordinaten des Mauspunktes.
- Grundlage ist weiterhin `TDMatPoint` im Core, nicht Screen-Pixel.

### Winkel

- Winkel wird aus der aktiven Hilfslinie berechnet:
  - Startpunkt: erster/letzter gesetzter relevanter Punkt der Operation
  - Endpunkt: aktueller Mauspunkt
- Berechnung in realen VED-Koordinaten:
  - `atan2(end.y - start.y, end.x - start.x)`
  - Ausgabe in Grad
- Der genaue Wertebereich soll festgelegt werden:
  - Vorschlag: `-180.0 ... +180.0`
  - Alternative: `0.0 ... 360.0`
- Die Entscheidung soll einheitlich fuer alle Operationen gelten.

### Abstand

- Abstand ist die Laenge der aktiven Hilfslinie in nativen VED-Koordinaten:
  - `sqrt((end.x - start.x)^2 + (end.y - start.y)^2)`
- Keine Umrechnung in mm/cm/inch in dieser Story.
- Spaetere Integration mit `UnitFormatter`/Page-Settings ist moeglich, aber nicht Bestandteil dieser Story.

## Architekturvorschlag

Die Berechnung gehoert in den VED-Core, nicht in Qt.

### Neue Core-Struktur

In `vop_base.h`:

```cpp
struct TDVOPMouseStatus {
    TDMatPoint point;
    bool hasPoint = false;

    double angleDeg = 0.0;
    bool hasAngle = false;

    double distance = 0.0;
    bool hasDistance = false;
};
```

### Erweiterung `TDViewOperation`

```cpp
virtual TDVOPMouseStatus GetMouseStatus() const;
```

Default:

- kein Punkt
- kein Winkel
- kein Abstand

### Gemeinsame Hilfsfunktion fuer Create-Operationen

In `TDVOCreate`:

```cpp
TDVOPMouseStatus HelperLineStatus(TDMatPoint start, TDMatPoint end) const;
```

Diese Methode setzt:

- `point = end`
- `hasPoint = true`
- `angleDeg`
- `hasAngle = true`
- `distance`
- `hasDistance = true`

Damit wird die Berechnung nicht pro Operation dupliziert.

### Operationen

Jede Operation, die eine aktive Hilfslinie hat, ueberschreibt `GetMouseStatus()` und liefert die fachlich richtige Linie.

Beispiele:

- `VOC_LINE`
  - Start: erster Linienpunkt
  - End: aktueller Mauspunkt
- `VOC_ELLIPSE_MIDPOINT`
  - Start: Mittelpunkt
  - End: aktueller Mauspunkt
- `VOC_POLYGON_SMARTLINE`
  - Start: letzter gesetzter Polygonpunkt
  - End: aktueller Mauspunkt
- `VOC_POLYCURVE`
  - Start: letzter relevanter Conturpunkt
  - End: aktueller Mauspunkt
- `VOC_RECTANGLE_ROTATED`
  - Start/End entsprechend der aktuell gezeichneten Hilfslinie der Operation

### Operation Manager

`TDViewOperationManager` bekommt:

```cpp
TDVOPMouseStatus GetMouseStatus() const;
```

Die Methode delegiert an die aktive Operation.

### Qt/UI

`MainWindow` zeigt die Werte nur an.

Aktuell:

```text
X: ...  Y: ...
```

Neu, wenn Winkel und Abstand vorhanden sind:

```text
X: ...  Y: ...  A: ... deg  D: ...
```

Wenn keine aktive Hilfslinie existiert, bleibt die Anzeige wie bisher:

```text
X: ...  Y: ...
```

## Wichtige Designentscheidung

Die Werte werden aus realen VED-Koordinaten berechnet, nicht aus Screen-Pixeln.

Damit sind sie unabhaengig von:

- Zoom
- Pan
- Qt-Rendering
- DPI
- GraphicEngine-Implementierung

## Nicht Bestandteil dieser Story

- Keine Umrechnung in mm/cm/inch.
- Keine Persistierung.
- Keine Aenderung der Operation-Geometrie.
- Keine Snap-/Constraint-Funktion fuer feste Winkel.
- Keine Eingabefelder zum manuellen Setzen von Winkel oder Abstand.

## Akzeptanzkriterien

- Bei den betroffenen Operationen erscheinen nach dem ersten gesetzten Punkt Winkel und Abstand in der Fusszeile.
- Die Werte aktualisieren sich bei jeder Mausbewegung.
- Select-/Move-/Delete-Operationen zeigen keinen Winkel/Abstand, solange sie keine aktive Hilfslinie definieren.
- Berechnung liegt im Core/Operation-Layer.
- Qt liest nur den Status und formatiert die Anzeige.
- Bestehende Tests bleiben gruen.
- Neue Tests pruefen mindestens:
  - Winkel/Abstand fuer eine einfache horizontale Hilfslinie
  - Winkel/Abstand fuer eine vertikale Hilfslinie
  - Operation ohne Hilfslinie liefert `hasAngle == false` und `hasDistance == false`
