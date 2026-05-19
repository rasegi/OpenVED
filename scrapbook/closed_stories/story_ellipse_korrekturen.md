# Story: Ellipse-Korrekturen

Datum: 2026-05-17
Status: geschlossen

## Kontext

Bei der Portierung der Ellipse-Operationen in den Qt-Port gab es mehrere Abweichungen
zum alten Borland-VED-Verhalten. Besonders betroffen waren rotierte Ellipsen:

- Auswahlrahmen in `VOM_SELECTMOVE_OBJECT_SCALE_FRAME` war bei Ellipsen anders als bei
  anderen Objekten.
- Node-Bearbeitung einer rotierten Ellipse veraenderte das Achsenverhalten unnatuerlich.
- Rotierte Ellipsen liessen sich zeitweise nicht oder nur unzuverlaessig erzeugen.
- Schnelle Klickfolgen konnten zufaellig dazu fuehren, dass der zweite Klick nicht als
  naechster VED-Schritt ankam.

Ziel war nicht eine neue Ellipse-Logik, sondern eine moeglichst VED-treue Korrektur im
Qt-Port.

## Ziel

Die Ellipse soll sich im Qt-Port wie im alten VED verhalten:

- Skalier-Auswahlrahmen soll bei `VOM_SELECTMOVE_OBJECT_SCALE_FRAME` fuer Ellipsen
  horizontal umrandend sein.
- Normale Node-Bearbeitung soll bei rotierten Ellipsen die Achsenrichtung erhalten und
  nur die Radien anpassen.
- Rotierte Ellipsen muessen ueber Mittelpunkt, Achsenpunkt und Radiuspunkt stabil
  erzeugt werden koennen.
- Schnelle zweite Klicks duerfen nicht von Qt-Doppelklick-Erkennung verschluckt werden.

## Umsetzung

### Auswahlrahmen fuer Scale-Frame-Operation

Fuer den Scale-Frame-Modus wurde ein operation-spezifischer Rahmen eingefuehrt:

- `TDVecObject::GetScaleFrame()` liefert standardmaessig `GetFrame()`.
- `TDVecEllipse::GetScaleFrame()` liefert fuer rotierte Ellipsen einen horizontalen
  Bounding-Frame.
- `TDVOMSelectMoveObjectScaleFrame` nutzt fuer diese Operation `GetScaleFrame()`,
  `PointOnScaleFrameNode()` und `MoveScaleFrameNode()`.

Damit bleibt das allgemeine Objektverhalten unangetastet, waehrend die Scale-Frame-
Operation fuer Ellipsen denselben horizontalen Rahmen zeigt wie bei anderen Objekten.

### Node-Bearbeitung rotierten Ellipsen

`TDVecEllipse::MoveNode()` wurde auf VED-naehere Semantik gebracht:

- Bei normaler Node-Bearbeitung bleibt der Winkel der Ellipse stabil.
- Das Verschieben eines Node veraendert die passenden Radien.
- Affine Skalierung bleibt weiterhin ueber `ToScale()` getrennt moeglich.

Dafuer wurden kleine Geometrie-Helfer ergaenzt, unter anderem fuer Frame-Nodes und
horizontale/vertikale Schnittberechnungen.

### Erzeugen rotierten Ellipsen

`TDVOCEllipseMidpoint` wurde korrigiert:

- Der dritte Klick berechnet die aktuelle Ellipse direkt aus Mittelpunkt, Achsenpunkt
  und aktuellem Klickpunkt.
- Der zweite Klick validiert die Achse jetzt gegen seinen echten Klickpunkt, nicht gegen
  einen moeglicherweise alten Preview-Punkt.
- `OPMouseMove()` aktualisiert den Preview-Punkt vor der Berechnung. Dadurch erscheinen
  Cursor 2 und Vorschau sofort beim ersten Move nach dem zweiten Klick.

Diese Korrektur beseitigt den Fall, dass die Operation scheinbar in Status/Cursor 1
haengen bleibt, obwohl der zweite Klick fachlich gueltig war.

### Schnelle Klickfolgen in Qt

Im Qt-Widget wurde `mouseDoubleClickEvent()` ergaenzt und bewusst wie ein normaler
`mousePressEvent()` behandelt.

Grund: Bei schnellem zweiten Klick kann Qt diesen Klick als Doppelklick-Event liefern.
Ohne Handler erreicht dieser Klick die VED-Operation nicht zuverlaessig. Das alte VED
zaehlt aber Klicks innerhalb der Operation; Qt darf diesen zweiten Klick daher nicht
verschlucken.

## Geaenderte Dateien

- `src/app/QVedWidget.h`
- `src/app/QVedWidget.cpp`
- `src/ved_core/operations/voc_ellipse_midpoint.cpp`
- `src/ved_core/operations/vom_selectmove_object_scale_frame.cpp`
- `src/ved_core/vecobjects/vec_ellipse.h`
- `src/ved_core/vecobjects/vec_ellipse.cpp`
- `src/ved_core/vecobjects/vec_object.h`
- `src/ved_core/vecobjects/vec_object.cpp`
- `src/ved_core/vecobjects/vec_object_geometry.h`
- `src/ved_core/vecobjects/vec_object_geometry.cpp`
- `tests/ved_core_circle_ellipse_operation_tests.cpp`
- `tests/ved_core_select_scale_frame_operation_tests.cpp`

## Akzeptanzkriterien

- `VOM_SELECTMOVE_OBJECT_SCALE_FRAME` zeigt bei Ellipsen einen horizontalen umrandenden
  Scale-Frame.
- Normale Node-Bearbeitung rotierten Ellipsen behaelt den Ellipsenwinkel bei.
- Rotierte Ellipsen koennen ueber die Mittelpunkt-Operation erzeugt werden.
- Der dritte Klick kann direkt eine gueltige rotierte Ellipse abschliessen.
- Der zweite Klick bleibt auch ohne vorherigen Preview-Move gueltig.
- Nach dem zweiten Klick wechselt die Operation beim ersten Preview-Move auf Cursor 2.
- Schnelle zweite Klicks werden vom Qt-Widget an die VED-Operation weitergegeben.

## Verifikation

Ausgefuehrt:

```text
cmake --build cmake-build-debug --target ved_core_circle_ellipse_operation_tests
ctest --test-dir cmake-build-debug --output-on-failure -R ved_core_circle_ellipse_operation_tests
ctest --test-dir cmake-build-debug --output-on-failure
cmake --build cmake-build-debug --target ved_qt_app
```

Ergebnis:

- Gezielter Ellipse-Test erfolgreich.
- Gesamte Testsuite erfolgreich: 20/20.
- `ved_qt_app` baut erfolgreich.

## Hinweise

Die Korrektur ist absichtlich getrennt nach Operationen:

- Normales Node-Move-Verhalten bleibt VED-nah fuer Objektbearbeitung.
- Scale-Frame nutzt einen eigenen Rahmen, damit die UI bei Ellipsen konsistent mit den
  anderen Objekten bleibt.
- Qt-Doppelklick wird zentral im Widget behandelt, weil dieses Problem nicht nur
  Ellipsen betreffen kann, sondern alle mehrstufigen VED-Operationen mit schnellen
  Klickfolgen.
