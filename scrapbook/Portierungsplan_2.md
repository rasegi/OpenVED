# Portierungsplan 2 - Minimale echte VED

Stand: 2026-05-14

## Ziel

Das naechste Ziel ist kein weiterer Qt-Slice, sondern eine minimale, echte VED:

- ein Line-Objekt erzeugen
- Line-Objekt selektieren
- Line-Objekt verschieben
- Line-Endpunkte aendern
- Line-Objekt loeschen
- Zeichnung in Datei speichern und wieder laden

Erst wenn dieser Durchstich fachlich stabil ist, werden weitere Operationen und
Objekttypen portiert.

## Aktueller Befund

Der aktuelle Port hat eine lauffaehige Qt-Huelle mit Canvas, Toolbar,
Grid/Zoom/Scroll, Basis-Rendering und `VOC_LINE`. Das ist technisch nuetzlich,
aber noch keine kleine echte VED, weil die Arbeitskette nach dem Erzeugen
abbricht:

- `TDVecLine` existiert.
- `TDVecModel` kann Objekte aufnehmen, zaehlen, liefern und deselektieren.
- `VOC_LINE` kann eine Linie ins Modell schreiben.
- Selektion existiert nur als Flag am Objekt, nicht als echte Bedienoperation.
- Hit-Testing fehlt.
- Move/Edit/Delete-Operationen fehlen.
- Datei-Speicherung/Laden fehlt.
- Undo/Redo, Mehrfachselektion, Clipboard, Text, Druck, komplexe Objekte und
  die meisten alten Operationen bleiben ausserhalb dieses Plans.

Die wichtige Architekturentscheidung: Fuer diese Phase wird die alte
Operation-Pipeline ernst genommen. Neue Qt-Hilfslogik darf nur Adapter sein;
fachliche Maus-Zustaende, Cursor und Modellmutationen sollen in Core-/Operation-
Klassen liegen.

## Definition of Done

Eine Version gilt als "minimale echte VED", wenn dieser Ablauf funktioniert:

1. App starten.
2. `VOC_LINE` aktivieren.
3. Linie mit zwei Punkten erzeugen.
4. Select-Operation aktivieren.
5. Linie anklicken, Selektion wird sichtbar.
6. Selektierte Linie verschieben.
7. Einen Linienendpunkt anfassen und verschieben.
8. Linie loeschen.
9. Neue Linie erzeugen.
10. Datei speichern.
11. App/Modell leeren.
12. Datei laden.
13. Linie ist wieder vorhanden, korrekt positioniert und selektier-/editierbar.

## Nicht-Ziele dieser Phase

- Keine weiteren Objekttypen ausser `TDVecLine`.
- Kein vollstaendiges altes Dateiformat, falls das den Durchstich blockiert.
- Kein Undo/Redo.
- Keine Mehrfachselektion.
- Keine komplexen Attribute ausser Farbe und Linienkoordinaten.
- Keine UI-Politur ueber das hinaus, was fuer die Bedienbarkeit notwendig ist.
- Keine weitere Toolbar-Breite ohne echte Operation dahinter.

## Port-Step-Log-Konvention

Alle folgenden Schritte werden unter `scrapbook/port_step_2_n.log`
dokumentiert.

`n` ist fortlaufend ab `001`.

Beispiel:

- `scrapbook/port_step_2_001.log`
- `scrapbook/port_step_2_002.log`
- `scrapbook/port_step_2_003.log`

Jeder Log enthaelt:

- Ziel des Steps
- geaenderte Dateien
- fachliche Entscheidung
- Test/Verifikation
- bekannte Restpunkte

## Phase 0 - Baseline sichern

### Step 2_001: Minimal-VED-Baseline dokumentieren

Ziel:

- aktuellen Stand als Ausgangspunkt festhalten
- klar markieren, welche Actions wirklich implementiert sind
- offene UI-Actions nicht als Fortschritt zaehlen

Arbeit:

- Build verifizieren
- aktuelles Verhalten von `VOC_LINE` pruefen
- Cursor-PNG-Portierung festhalten
- bekannte Grenzen im Log dokumentieren

Akzeptanz:

- Build laeuft
- `VOC_LINE` erzeugt weiterhin `TDVecLine`
- keine neue Funktion, nur belastbare Baseline

## Phase 1 - Line-Objekt fachlich komplettieren

### Step 2_002: `TDVecLine` fuer Hit-Testing und Mutation erweitern

Ziel:

- Core-Objekt kann beantworten, ob Mauspunkt Linie oder Node trifft.
- Core-Objekt kann verschoben und an Endpunkten geaendert werden.

Arbeit:

- `TDVecLine::HitTestLine(point, tolerance)`
- `TDVecLine::HitTestNode(point, tolerance)`
- `TDVecLine::MoveBy(dx, dy)`
- `TDVecLine::MoveNode(nodeId, point)`
- kleine Typen fuer Hit-Ergebnis einfuehren

Akzeptanz:

- Unit- oder kleine Core-Tests fuer Treffer auf Linie, Startpunkt, Endpunkt,
  Mittelpunkt und Nichttreffer
- keine Qt-Abhaengigkeit in `ved_core`

### Step 2_003: Modell-Selektion und Objektloeschung

Ziel:

- `TDVecModel` verwaltet eine echte Auswahl und kann Objekte entfernen.
- Die Modell-Suche arbeitet mit den von der View bereits gesnappten
  Real-Koordinaten.

Arbeit:

- `FindObjectAt(point, tolerance)`
- `FindNodeAt(point, tolerance)`
- `GetSelectedObject()`
- `DeleteSelectedObjects()` oder fuer Phase 2 mindestens `DeleteObject(index)`
- Selektionswechsel zentral im Modell

Akzeptanz:

- Linie kann programmgesteuert selektiert und geloescht werden.
- Selektion ist eindeutig sichtbar ueber `TDVecObject::GetSelect()`.

## Phase 2 - Minimal-Operationen portieren

### Step 2_004: Select-Operation fuer Line

Ziel:

- eigener Operation-Modus fuer Objektselektion.
- alte Cursor-/GridLock-Semantik erhalten: Select nutzt den `select_object`
  Cursor, und bei GridLock liegt der Mouse-Toleranzkreis auf der gesnappten
  Real-Koordinate.

Arbeit:

- alte `VOM_SELECT_OBJECT` oder kleine kompatible Minimalvariante portieren
- Operation in `TDViewOperationManager::SetOperation` registrieren
- Toolbar-Select wirklich an Operation binden
- Cursor `VECVIEW_CURSOR_DEFAULT` und `VECVIEW_SELECT_OBJECT` muessen auf
  denselben `select_object` Cursor zeigen
- `QVedWidget::operationPoint()` darf GridLock nicht nur fuer `VOC_LINE`
  anwenden; alle Operationen muessen die gesnappte Koordinate erhalten
- der kleine Lock-/Mouse-Toleranz-Kreis wird als Overlay ueber
  `DrawMouseToleranz()` gezeichnet, nicht in das Cursor-Bitmap eingebrannt

Akzeptanz:

- Klick auf Linie selektiert sie.
- Klick auf leere Flaeche deselektiert.
- Selektierte Linie zeichnet Nodes/Frame.
- Bei aktivem GridLock liegt der sichtbare Toleranzkreis und die
  Selektionskoordinate auf demselben Gridpunkt.

### Step 2_005: Move-Operation fuer Line

Ziel:

- selektierte Linie kann per Drag bewegt werden.

Arbeit:

- `VOM_MOVE_OBJECT` minimal fuer `TDVecLine`
- Drag-Start/Drag-Move/Drag-End in Operation
- temporare Vorschau ueber bestehende Tmp-Zeichenlogik oder direkte
  Modellaktualisierung mit Refresh

Akzeptanz:

- Linie bleibt selektiert.
- Drag verschiebt beide Endpunkte.
- Abbruch mit rechter Maustaste oder Escape setzt stabilen Zustand.

### Step 2_006: Node-Move-Operation fuer Line-Endpunkte

Ziel:

- Start- und Endpunkt der Linie koennen editiert werden.

Arbeit:

- `VOM_MOVE_NODE` minimal fuer `TDVecLine`
- Hit-Test fuer Start-/Endnode
- Drag setzt den betroffenen Endpunkt
- optional Mittelpunkt als Move-Ganzobjekt behandeln oder ignorieren

Akzeptanz:

- Endpunkt 1 und Endpunkt 2 lassen sich unabhaengig bewegen.
- Linie bleibt gueltig, Mindestlaenge wird beachtet.

### Step 2_007: Delete-Operation fuer Line

Ziel:

- selektierte Linie kann geloescht werden.

Arbeit:

- `VOM_DELETE_OBJECT` minimal oder Delete-Key-Handling
- Modell-Remove implementieren und UI refreshen

Akzeptanz:

- selektierte Linie verschwindet aus Modell und Canvas.
- Delete auf leerer Auswahl ist stabil.

## Phase 3 - Persistenz fuer Minimal-VED

### Step 2_008: Einfaches eigenes Minimal-Dateiformat

Ziel:

- Speichern/Laden blockiert nicht an altem proprietaeren Format.
- Format ist klar, testbar und spaeter migrierbar.

Vorschlag Format `*.ved2`:

```text
VED2 1
AREA 0 0 210000 296985
LINE x1 y1 x2 y2 color selected
```

Arbeit:

- `TDVecModelSerializer` oder aehnlicher Core-Service
- Save: alle `TDVecLine` schreiben
- Load: Modell leeren, Linien neu erzeugen
- robuste Fehlerbehandlung fuer falsche Header/Typen

Akzeptanz:

- Core-Test: Linie speichern, laden, Koordinaten/Farbe vergleichen.
- Datei ist menschenlesbar.

### Step 2_009: Save/Open in Qt-App anbinden

Ziel:

- Toolbar `Save` und `Open` funktionieren fuer `*.ved2`.

Arbeit:

- `QFileDialog` anbinden
- Serializer aufrufen
- Statusbar-Meldungen
- Dirty-State optional, aber nicht blockierend

Akzeptanz:

- Datei speichern
- Modell leeren oder App neu starten
- Datei laden
- Linie erscheint wieder korrekt.

## Phase 4 - Durchstich-Test

### Step 2_010: Manuelle und automatisierte End-to-End-Verifikation

Ziel:

- Minimal-VED als zusammenhaengender Workflow absichern.

Arbeit:

- Core-Tests fuer Model/HitTest/Serializer
- manueller Testplan im Log
- optional kleines Qt-smoke-testbares Szenario ohne pixelgenaue UI-Pruefung

Akzeptanz:

- kompletter Ablauf aus der Definition of Done funktioniert.
- bekannte Bugs sind dokumentiert und nicht workflow-blockierend.

## Prioritaeten

1. Core zuerst: Line-HitTest, Mutation, Modell-Selektion, Delete.
2. Operationen danach: Select, Move, NodeMove, Delete.
3. Persistenz danach: eigenes minimales Format.
4. UI nur dort erweitern, wo Operationen wirklich funktionieren.

## Risikoanalyse

### Risiko: Alte Operationen haengen an altem Modellumfang

Viele alte `VOM_*` Operationen erwarten APIs, die im aktuellen Minimalmodell
noch fehlen.

Massnahme:

- nur benoetigte Modell-APIs portieren
- alte Operationen nicht blind kopieren
- pro Operation zuerst Abhaengigkeiten listen

### Risiko: Qt-Adapter bekommt zu viel Fachlogik

Wenn Hit-Test, Selektion und Move im Widget landen, entsteht wieder ein
Qt-Slice.

Massnahme:

- `QVedWidget` leitet Eingaben weiter
- Operationen veraendern Modell
- Modell/Objekte enthalten fachliche Geometrie

### Risiko: Dateiformat-Diskussion blockiert Fortschritt

Das alte Format kann spaeter wichtig werden, ist aber fuer Minimal-VED nicht
noetig.

Massnahme:

- erst `*.ved2` als bewusstes Minimalformat
- spaeter Import/Export altes Format als eigene Phase

## Ergebnis dieses Plans

Nach `port_step_2_010` gibt es eine kleine, aber echte VED:

- ein echtes Modell
- ein echtes Objekt
- echte Operationen fuer Create/Select/Move/Edit/Delete
- echte Persistenz
- Qt nur als UI-Adapter

Danach koennen weitere Objekte und Operationen einzeln in dieselbe Pipeline
eingehangen werden.
