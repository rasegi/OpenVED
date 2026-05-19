# Story: Qt-UI modularisieren und MainWindow entlasten

Datum: 2026-05-16
Aktualisiert: 2026-05-17

## Kontext

Der aktuelle Qt-Port ist bewusst pragmatisch gewachsen:

- VED-Core und Operationen wurden schrittweise portiert.
- Toolbar-Aktionen, Docks, Statuszeile, Canvas-Anbindung und UI-State-Logik wurden zuerst zentral in `MainWindow` zusammengefuehrt.
- Dadurch ist die Anwendung lauffaehig, aber `MainWindow.cpp` ist inzwischen zu gross geworden.

Aktueller Zustand:

- `src/app/MainWindow.cpp`: ueber 2000 Zeilen
- `src/app/QVedWidget.cpp`: ueber 1000 Zeilen
- `MainWindow` enthaelt mehrere fachlich unterschiedliche Verantwortungen:
  - App-Shell
  - Menus und Toolbars
  - VED-Core-/OperationManager-Anbindung
  - Operation-zu-QAction-Mapping
  - Statuszeile
  - PolyCurve-/B-Spline-Dock
  - RoundRectangle-Dock
  - Rotate-Dock
  - Text-/FrameText-Dock
  - Enable-/Disable-Regeln fuer Modify-Aktionen
  - Cursor-Policy fuer Mouse-Tolerance-Cross
  - Settings und Window-State

Das ist fuer einen fruehen Port akzeptabel, soll aber nicht zur dauerhaften Architektur werden.

## Problem

Eine zu grosse `MainWindow`-Klasse fuehrt zu mehreren Risiken:

- UI-Bugs entstehen durch Kopplung eigentlich getrennter Themen.
  - Beispiel: Cursor-Policy, Select-Modus und Auswahlrechteck beeinflussen sich gegenseitig.
- Neue Operationen fuegen immer mehr Toolbar- und State-Code in dieselbe Datei ein.
- Feature-Docks werden schwer testbar, weil sie direkt in `MainWindow` eingebettet sind.
- Spaetere Plugin-Integration wird schwieriger, wenn `MainWindow` alle Actions, Docks und Toolbars selbst verwaltet.
- Es wird unklar, welche Logik Qt-spezifisch ist und welche nur als Adapter zum VED-Core dient.

Qt verlangt keine grosse `MainWindow.cpp`. Eine kleine MainWindow-Klasse als Kompositionswurzel ist ueblicher und besser wartbar.

## Ziel

Die Qt-UI soll modularisiert werden, ohne VED-Core, Mathematik, Objektmodell oder Operationen fachlich zu veraendern.

Ziele:

- `MainWindow` wird zur App-Shell und Kompositionswurzel reduziert.
- Toolbars, Docks und UI-State-Regeln werden in eigene Qt-Komponenten verschoben.
- VED-Core bleibt Qt-frei.
- Operationen bleiben weiterhin im Core bzw. in der bestehenden Operation-Schicht.
- Bestehendes Verhalten bleibt unveraendert.
- Refaktorierung erfolgt inkrementell und testbar.

## Nicht-Ziele

- Keine Aenderung an VED-Mathematik.
- Keine Aenderung an Objektgeometrie.
- Keine Aenderung am Persistenzformat.
- Keine neue Plugin-Architektur in dieser Story.
- Keine Umbenennung von `VOC_*` oder `VOM_*`.
- Keine fachliche Neuinterpretation alter Borland-VED-Operationen.

## Architekturvorschlag

### Zielstruktur

```text
src/app
  MainWindow
    App-Shell, Menus, zentrale Komposition

  VedActionController
    QAction -> TDViewOperationEnum
    QAction -> QVedWidget::InteractionMode
    Status-Text
    Aktivieren/Deaktivieren von Zoom/Create/Modify Modi

  VedToolbars
    FileToolbar
    CreateToolbar
    ModifyToolbar
    ViewToolbar

  VedUiStateController
    Group-Actions enabled/disabled
    Modify-Actions enabled/disabled
    Mouse-Tolerance-Cross-Cursor-Policy
    Auswahlabhaengige UI-Regeln

  CurveDock
    PolyCurve/B-Spline/Modify-Curve-Attribute UI

  RoundRectangleDock
    VOC_ROUND_RECTANGLE_NOTROTATED UI

  RotateDock
    VOM_ROTATE_OBJECT_ACTIVEPOINT UI

  TextDock
    cmdVOCVecText / cmdVOMVecText UI
    schreibt nur ExtVars bzw. bestehende Textobjekt-Parameter

  QVedWidget
    Canvas, Mouse/Keyboard Forwarding, Paint Surface
```

## Komponenten

### 1. MainWindow

`MainWindow` soll nur noch:

- zentrale Widgets erzeugen,
- Komponenten verbinden,
- Settings lesen/schreiben,
- Window-State verwalten,
- globale Menus initialisieren,
- App-Lifecycle behandeln.

`MainWindow` soll nicht dauerhaft enthalten:

- grosse Toolbar-Aufbau-Logik,
- Dock-internes Formularverhalten,
- detaillierte Enable-/Disable-Regeln,
- operationstyp-spezifische UI-Sonderfaelle.

### 2. VedActionController

Diese Komponente verwaltet die Verbindung zwischen Qt-Actions und VED-Operationen.

Moegliche Verantwortung:

```cpp
class VedActionController {
public:
    void ActivateCreateOperation(TDViewOperationEnum op, QString opName);
    void ActivateModifyOperation(TDViewOperationEnum op, QString opName);
    void ActivateSelectOperation();
    void ActivateSelectScaleOperation();
    void ActivateMoveObjectOperation();
    void ActivateDeleteObjectOperation();
    void DeactivateCurrentOperation();
};
```

Sie kennt:

- `TDVecEditCad`
- `TDViewOperationManager`
- `QVedWidget`
- Status-Callback

Sie soll keine Dock-Widgets bauen.

### 3. VedToolbars

`VedToolbars` baut die sichtbaren Toolbars und stellt die erzeugten Actions strukturiert bereit.

Beispiel:

```cpp
struct VedToolbarActions {
    QAction* select = nullptr;
    QAction* selectScale = nullptr;
    QAction* moveObject = nullptr;
    QAction* insertNode = nullptr;
    QAction* deleteNode = nullptr;
    QAction* changeEdgeRoundNode = nullptr;
    QAction* modifyCurveAttribute = nullptr;
    QAction* makeGroup = nullptr;
    QAction* resolveGroup = nullptr;
    QAction* zoom = nullptr;
    QAction* showRulers = nullptr;
    QAction* showMouseToleranceCross = nullptr;
};
```

Die Toolbars koennen weiterhin klassische Qt-Widgets sein. Wichtig ist, dass das Mapping nicht mehr als hunderte Zeilen in `MainWindow.cpp` steht.

### 4. VedUiStateController

Diese Komponente entscheidet, welche Aktionen gerade aktivierbar sind.

Beispiele:

- `Insert Node` nur bei genau einem selektierten passenden Objekt.
- `Delete Node` nur bei genau einem selektierten passenden Objekt.
- `Modify Curve Attribute` nur bei genau einer selektierten B-Spline oder PolyCurve.
- `Make Group` nur bei Mehrfachauswahl.
- `Resolve Group` nur wenn selektierte Gruppen vorhanden sind.
- Mouse-Tolerance-Cross darf `VECVIEW_DOCK` und `VECVIEW_NO` nicht ueberschreiben.

Diese Regeln sind UI-Regeln, aber sie fragen Core-Zustand ab. Deshalb gehoeren sie in eine Qt-nahe Adapter-Komponente, nicht in den Core.

### 5. CurveDock

Das PolyCurve-/B-Spline-Dock soll eine eigene Klasse werden.

Verantwortung:

- Widgets erzeugen,
- Werte anzeigen,
- Werte in `TDVOCCurveExtVar` bzw. Modify-Curve-Operation schreiben,
- Cancel/Apply behandeln,
- Sichtbarkeit je aktiver Operation steuern.

Moeglicher Name:

```text
src/app/docks/CurveDock.h
src/app/docks/CurveDock.cpp
```

### 6. RoundRectangleDock

Das RoundRectangle-Dock soll eine eigene Klasse werden.

Verantwortung:

- Radius, Resolution, Show-Controls, Show-Polygon darstellen,
- Werte in `TDVOCRoundRectNotRotExtVar` schreiben,
- Breite/Hoehe aus der ExtVar anzeigen,
- Dock nur bei `VOC_ROUND_RECTANGLE_NOTROTATED` anzeigen.

Moeglicher Name:

```text
src/app/docks/RoundRectangleDock.h
src/app/docks/RoundRectangleDock.cpp
```

### 7. RotateDock

Das Rotate-Dock soll ebenfalls eine eigene Klasse werden.

Verantwortung:

- Winkel, Copy, Select Copy darstellen,
- Pivot und Objektanzahl anzeigen,
- Werte in `TDVOMRotateObjectActPtExtVar` schreiben,
- Dock nur bei `VOM_ROTATE_OBJECT_ACTIVEPOINT` anzeigen.

Moeglicher Name:

```text
src/app/docks/RotateDock.h
src/app/docks/RotateDock.cpp
```

### 8. TextDock

Das Text-Dock soll eine eigene Klasse werden.

Verantwortung:

- Textinhalt, Skalierung, Winkel, Neigung, Spacing, Resolution, Ausrichtung und Flags darstellen,
- bei `cmdVOCVecText` ausschliesslich `TDVOCVecTextExtVar` aktualisieren,
- bei `cmdVOMVecText` das selektierte `TDVecText` / `TDVecFrameText` bearbeiten,
- Font-Auswahl zunaechst deaktiviert lassen, solange Phase 3 nur `wps_default.vfn` nutzt,
- bei `cmdVOCVecFrameTextEmpty` keine eigene Box-Erzeugung implementieren.

Wichtiger Grundsatz:

- Text- und FrameText-Erzeugung bleibt in den Borland-nahen Core-Operationen:
  - `TDVOCVecText`
  - `TDVOCVecFrameTextEmpty`
- Das Qt-Dock ist nur Editor fuer Dialogwerte/ExtVars.
- Keine Text- oder FrameText-Erzeugungslogik darf wieder in `MainWindow` oder ein Dock wandern.

Moeglicher Name:

```text
src/app/docks/TextDock.h
src/app/docks/TextDock.cpp
```

### 9. QVedWidget spaeter trennen

`QVedWidget.cpp` ist ebenfalls gross. Diese Story priorisiert zuerst `MainWindow`, aber danach sollte `QVedWidget` analysiert werden.

Moegliche spaetere Trennung:

- Canvas-Painting
- Scroll-/Zoom-Verhalten
- Mouse-/Keyboard-Event-Forwarding
- Cursor- und Marker-Zeichnung
- Grid/Ruler-View-Hilfen

Grid und Ruler sind aktuell Qt-Zeichnung, aber fachlich nahe an View-Hilfen. Langfristig koennen sie eigene View-Overlay-Komponenten werden.

## Reihenfolge der Umsetzung

### Schritt 0: Risikoarmer Dateisplit

Vor der eigentlichen Architekturaenderung kann die bestehende `MainWindow`-Implementierung mechanisch auf mehrere `.cpp`-Dateien verteilt werden:

- `MainWindow.cpp`
- `MainWindowToolbars.cpp`
- `MainWindowCurveDock.cpp`
- `MainWindowRoundRectangleDock.cpp`
- `MainWindowRotateDock.cpp`
- `MainWindowTextDock.cpp`

Dabei bleibt `MainWindow` dieselbe Klasse und `MainWindow.h` bleibt die zentrale Deklaration.

Grund:

- sofortige Reduktion von `MainWindow.cpp`,
- sehr geringe fachliche Aenderungsgefahr,
- gute Vorbereitung fuer echte Dock-/Controller-Klassen.

### Schritt 1: Docks als eigene Klassen auslagern

- `CurveDock`
- `RoundRectangleDock`
- `RotateDock`
- `TextDock`

Grund:

- klar abgegrenzte Verantwortung,
- wenig Risiko fuer Operation-Mapping,
- sofortige Reduktion von `MainWindow.cpp`.

### Schritt 2: Toolbar-Aufbau extrahieren

- `VedToolbars`
- Action-Sammlung als Struktur zurueckgeben.

Grund:

- `createToolBars()` ist der groesste Block in `MainWindow.cpp`.
- Neue Operationen koennen danach datengetriebener hinzugefuegt werden.

### Schritt 3: Operation-Action-Mapping extrahieren

- `VedActionController`
- einheitliche Aktivierung fuer Create/Modify/Select/View-Operationen.

Grund:

- aktuell gibt es viele aehnliche Lambdas.
- Wiederholung erhoeht Bugrisiko.

### Schritt 4: UI-State-Regeln extrahieren

- `VedUiStateController`
- ersetzt `updateGroupCommandActions`, `updateModifyCommandActions`, `updateMouseToleranceCrossCursor`.

Grund:

- Auswahlabhaengige Regeln werden wachsen.
- Plugin-System wird spaeter eigene Regeln anmelden wollen.

### Schritt 5: QVedWidget analysieren

- eigene Story oder Folgeschritt.
- Ziel: Canvas-Verantwortungen klar trennen.

## Akzeptanzkriterien

- `MainWindow.cpp` ist deutlich kleiner und enthaelt primaer Komposition.
- Docks sind eigene Klassen.
- Toolbar-Erzeugung ist aus `MainWindow` ausgelagert.
- Operation-Aktivierung geschieht ueber eine zentrale Controller-Komponente.
- Enable-/Disable-Regeln sind an einer Stelle gebuendelt.
- VED-Core erhaelt keine Qt-Abhaengigkeiten.
- Bestehende Tests laufen unveraendert.
- Keine Veraenderung an Geometrie, Mathematik oder Borland-VED-Verhalten.

## Teststrategie

- Bestehende Core-Tests muessen weiter bestehen.
- Qt-nahe Tests koennen fuer Action-/Cursor-Regeln ergaenzt werden.
- Kritische manuelle Checks:
  - alle Toolbar-Buttons aktivieren korrekte Operation,
  - Statuszeile zeigt aktive Operation,
  - PolyCurve/B-Spline-Dock erscheint nur bei passenden Operationen,
  - RoundRectangle-Dock erscheint nur bei RoundRectangle-Operation,
  - Rotate-Dock erscheint nur bei Rotate-Operation,
  - Text-Dock schreibt bei `cmdVOCVecText` nur `TDVOCVecTextExtVar`,
  - `cmdVOCVecFrameTextEmpty` erzeugt die Box weiterhin ueber `TDVOCVecFrameTextEmpty`,
  - Mouse-Tolerance-Cross ueberschreibt Drag-Cursor nicht,
  - Select/Move/Node-Move zeigen kein falsches Auswahlrechteck,
  - Grid/Ruler/Zoom bleiben unveraendert.

## Beziehung zu anderen Stories

- `story_plugin_architecture_persistent_semantics.md`
  - Diese UI-Modularisierung erleichtert spaeter Plugin-UI, Actions und Docks.
- `story_object_list_and_attribute_editors.md`
  - Neue Object-List- und Attribute-Docks sollten direkt als eigene Qt-Komponenten entstehen.
- `story_layers_and_object_order.md`
  - Layer-UI sollte nicht mehr in `MainWindow` eingebaut werden, sondern als eigenes Dock/Panel.

## Grundsatz

Diese Story ist ein Qt-Schicht-Refactoring.

Der VED-Core bleibt stabil, Qt-frei und Borland-VED-treu.
