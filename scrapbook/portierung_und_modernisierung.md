# Portierung und Modernisierung von `borland_ved`

Stand: 2026-05-14

## Ausgangslage

Der alte `borland_ved`-Code enthaelt einen wertvollen 2D-CAD-Kern aus Borland-C++/VCL-Zeit. Ziel ist nicht, die alte VCL-Anwendung weiterzupflegen, sondern den Kern unter Qt wieder lauffaehig zu machen und danach bzw. parallel auf modernes C++23 zu bringen.

Wichtige Entscheidung: **Alte Datei-/Stream-Kompatibilitaet ist nicht erforderlich.** Das reduziert den Aufwand deutlich, weil die alte Persistenzschicht nicht konserviert werden muss.

## Konsequenz der fehlenden Rueckwaertskompatibilitaet

Folgende Altlasten koennen entfallen oder radikal ersetzt werden:

- `TDStream`, `TDSimpleInStream`, `TDSimpleOutStream`
- `TDStreamable` als zwingende Basisklasse fuer Persistenz
- alte binaere Factory-IDs wie `'line'`, `'grup'`, `'elip'`
- altes binaeres Streamformat
- Borland-/Windows-spezifische Clipboard-Serialisierung
- spezielle Stream-Roundtrip-Kompatibilitaet mit alten Dateien

Neue Persistenz kann spaeter frei entworfen werden, z.B. JSON, CBOR, SQLite, XML oder ein eigenes versioniertes Format.

## Zielarchitektur

```text
ved_core
  geometry/
  model/
  objects/
  operations/
  rendering_interface/
  resources/

ved_qt
  QVedWidget
  TDGraphicEngineQt oder modernes Rendering-Interface
  QtCursorRegistry
  QtClipboardAdapter
  QtCommand/Tool Adapter

ved_app
  kleine Qt-Testanwendung
  Toolbar fuer wichtigste CAD-Operationen
  spaeter Multi-View, Property Panels, Persistenz
```

## Grundstrategie

Nicht zuerst alles modernisieren. Zuerst einen kleinen, lauffaehigen Kern unter Qt erreichen, dann schrittweise modernisieren. Da alte Persistenz nicht wichtig ist, darf die Stream-Schicht frueh entfernt oder ausgeklammert werden.

Empfohlene Reihenfolge:

1. Qt-Port mit minimalem Kern starten.
2. Alte Persistenz komplett aus dem ersten Build herauslassen.
3. `sclib`-Container frueh durch STL ersetzen.
4. Objekt-Ownership auf RAII umstellen.
5. Qt-Rendering und Qt-Input sauber anbinden.
6. Neue Persistenz erst definieren, wenn Modell und Objektstruktur stabil sind.

## Phase 1: Minimalen Core isolieren

Ziel: CAD-Kern ohne VCL, ohne GDI-Screen, ohne alte Persistenz kompilierbar machen.

Einbeziehen:

- `src/comp/mathlib`
- `src/comp/vecobjects`
- `src/comp/main` soweit ohne Clipboard/Persistenz
- `src/comp/operations`
- `src/comp/lib`
- abstrakte Grafik-Schnittstelle aus `src/comp/gengine/vec_graphic_engine.*`

Ausklammern:

- `src/comp/vcl`
- `src/vedtest`
- `src/wps`
- `vec_graphic_engine_screen`
- `vec_graphic_engine_printer`
- `vec_clipdata` zunaechst
- alte Stream-Dateien, sofern sie nur fuer Persistenz benoetigt werden

Erwartete Aufgaben:

- Borland-Makros neutralisieren: `__fastcall`, `PACKAGE`, `__property`, `__published`
- alte Includes ersetzen: `values.h`, `windows.h`, `malloc.h`
- CMake-Ziel fuer `ved_core` anlegen
- klare Include-Pfade schaffen

## Phase 2: STL statt `sclib`-Container

Die leicht ersetzbaren `sclib`-Typen:

| Alt | Ersatz |
|---|---|
| `TDArrayImp` | entfaellt |
| `TDArray<T>` | `std::vector<T>` |
| `TDObjArray<T>` | `std::vector<T>` |
| `TDEasyArray<T>` | `std::vector<T>` |
| `TDObjPtrArray<T>` | `std::vector<std::unique_ptr<T>>` oder temporaer `std::vector<T*>` |
| `TDDynamicFactory` | `std::unordered_map<Id, std::function<...>>` |

Wichtigster Punkt ist `TDObjPtrArray<T>`. Dieses Array besitzt die eingefuegten Objekte und loescht sie im Destruktor. Der moderne Zieltyp ist meistens:

```cpp
std::vector<std::unique_ptr<TDVecObject>>
```

Wenn fuer eine Uebergangsphase zu viel Code betroffen ist, kann man temporaer Adapter verwenden. Ziel sollte aber echtes RAII sein, nicht nur ein Wrapper mit alter Semantik.

## Phase 3: Objektmodell modernisieren

Ziel: manuelle Ownership entfernen.

Aktueller Stil:

- rohe Pointer
- `new` / `delete`
- `Clone()` gibt rohen Pointer zurueck
- `DropRemove()`, `DropDelete()`, `Replace()` transferieren Besitz manuell

Zielstil:

```cpp
class TDVecObject {
public:
    virtual ~TDVecObject() = default;
    virtual std::unique_ptr<TDVecObject> clone() const = 0;
};
```

Modell-Container:

```cpp
std::vector<std::unique_ptr<TDVecObject>> objects_;
```

Fuer polymorphe Kopien:

```cpp
auto copy = object->clone();
```

Besonders vorsichtig pruefen:

- Gruppen (`TDVecGroup`)
- temporaere Objekte in Operationen
- Undo-/Insert-/Delete-Operationen
- selektierte Objekte und Indexlisten
- alte `Drop...`-Semantik

## Phase 4: Qt-Rendering

Alte GDI-Klasse ersetzen:

- nicht `TDGraphicEngineScreen` portieren
- neue Qt-Implementierung schreiben

Moegliche Klasse:

```cpp
class TDGraphicEngineQt : public TDGraphicEngine
```

Mapping:

| Alt | Qt |
|---|---|
| `DrawLine` | `QPainter::drawLine` |
| `DrawEllipse` | `QPainter::drawEllipse` |
| `DrawRectangle` | `QPainter::drawRect` |
| `DrawPolygon` | `QPainter::drawPolygon` |
| `DrawNode` | kleine Pixmap oder Primitive |
| `TDRgbColor` | `QColor` |
| Pan/Zoom/Margin | `QTransform` oder eigene Koordinatentransformation |

Wichtig: Die alte `TDGraphicEngineScreen` enthaelt nicht nur Zeichnung, sondern auch Zoom, Pan, DPI, ViewRange, WorkSpaceRange und Mouse-Toleranzen. Diese Semantik muss bewusst nachgebaut werden.

## Phase 5: Qt-Widget und Input

Neue UI-Schicht:

```cpp
class QVedWidget : public QWidget
```

Aufgaben:

- `paintEvent` ruft Rendering des Modells auf
- `mousePressEvent`, `mouseMoveEvent`, `mouseReleaseEvent` auf CAD-Operationen mappen
- `keyPressEvent`, `keyReleaseEvent` auf virtuelle Keys mappen
- Cursorwechsel ueber `QWidget::setCursor`
- Zoom, Pan, Grid, Selektion anzeigen

Die alte VCL-Klasse `TSCVecView` dient als Referenz, nicht als Portierungsziel.

## Phase 6: Operationen erhalten, aber modern anbinden

Die Operationen (`VOC_*`, `VOM_*`) sind fachlich wertvoll. Sie sollten nicht neu erfunden werden, sondern zuerst weiterverwendet werden.

Zu erhalten:

- Create Line
- Create Polygon/PolyCurve
- Create Circle/Ellipse/Rectangle
- Select/Move/Delete
- Move Node
- Insert/Delete Node
- Rotate/Scale
- Zoom

Modernisierung:

- `enum class` statt unscoped enum, wenn der Code stabil ist
- klare Input-Strukturen statt langer Parameterlisten
- kein `__fastcall`
- Operationen bekommen Modell/Editor als Referenzen oder Smart Pointer, nicht lose rohe Pointer

## Phase 7: Ressourcen und Cursor

Alte Ressourcen:

- `.cur` fuer CAD-Cursor
- `.bmp` fuer Toolbar-Icons
- `.bmp` fuer Nodes
- `.vfn` fuer alten Vektorfont

Qt-Ziel:

- Cursor-Registry: `TDVecViewCursor -> QCursor`
- Icons mittelfristig nach PNG konvertieren
- Nodes entweder als PNG oder direkt per QPainter zeichnen
- `.vfn` nur uebernehmen, wenn alter Vektorfont fachlich gebraucht wird

Da alte Dateikompatibilitaet nicht wichtig ist, kann auch der Font-Teil spaeter neu bewertet werden.

## Phase 8: Neue Persistenz

Erst einfuehren, wenn das moderne Objektmodell stabil ist.

Moegliche Formate:

- JSON: gut lesbar, einfach fuer Debugging
- CBOR: kompakter, Qt kann es gut
- SQLite: sinnvoll, wenn spaeter viele Objekte, Layer, Versionen, Metadaten kommen
- eigenes versioniertes Format: nur wenn Performance/Kompatibilitaet zwingend ist

Empfehlung fuer den Start:

- JSON oder CBOR
- explizite Versionsnummer
- stabile Objekt-IDs als Strings, z.B. `"line"`, `"ellipse"`, `"polycurve_area"`
- keine C++-Typnamen im Format speichern

## Tests

Ohne Tests ist die Modernisierung riskant. Mindesttests:

- Linie erzeugen, Frame pruefen
- Polygon/PolyCurve erzeugen, Punkte pruefen
- PolyCurveArea schliesst Kontur korrekt
- Move Object
- Move Node
- Rotate/Scale
- Selektion einzelner Objekte
- Gruppieren/Aufloesen
- Operation `VOC_LINE` mit synthetischen Maus-Events
- neue Persistenz: Save/Load Roundtrip

## Aufwandsschaetzung

Da alte Persistenz nicht erhalten werden muss:

| Ziel | Aufwand |
|---|---:|
| C++23-kompilierbarer Core ohne alte Persistenz | ca. 1-3 Wochen |
| Qt-Widget plus erste CAD-Funktionen lauffaehig | ca. 4-8 Wochen |
| Sauber modernisiert mit STL, RAII, neuer Persistenz und Tests | ca. 2-3 Monate |

Der groesste Brocken bleibt Ownership:

- `TDObjPtrArray`
- `Clone()`
- `DropRemove()`
- `DropDelete()`
- manuelle Objektlisten
- temporaere Objekte in Operationen

## Priorisierte Arbeitspakete

1. CMake/Qt-Projektstruktur anlegen.
2. `ved_core` ohne VCL/GDI/Persistenz bauen.
3. Borland-Kompatibilitaetsheader nur als Uebergang einfuehren.
4. `TDArray`/`TDObjArray`/`TDEasyArray` durch `std::vector` ersetzen.
5. `TDObjPtrArray<TDVecObject>` auf `std::vector<std::unique_ptr<TDVecObject>>` migrieren.
6. `Clone()` auf `std::unique_ptr` umstellen.
7. `TDGraphicEngineQt` schreiben.
8. `QVedWidget` schreiben.
9. wichtigste Operationen anbinden: Select, Line, Rectangle, PolyCurve, Move, Delete, Zoom.
10. Tests fuer Geometrie und Operationen ergaenzen.
11. neue Persistenz definieren.

## Leitentscheidung

Der alte Code soll nicht mechanisch uebersetzt werden. Der richtige Schnitt ist:

- CAD-Fachlogik behalten
- VCL/GDI/alte Streams entfernen
- Container und Ownership modernisieren
- Qt-Adapter neu schreiben
- neue Persistenz bewusst und versioniert entwerfen

Damit bleibt der fachliche Wert des alten Systems erhalten, ohne die technischen Altlasten dauerhaft mitzunehmen.
