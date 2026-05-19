# Bestandsaufnahme: `borland_ved` Legacy 2D-CAD Library

Stand: 2026-05-13

## Kurzfazit

Unter `borland_ved` liegt ein Borland-C++/VCL-Codebestand aus ca. 1999/2000. Der fachliche Kern ist besser getrennt als bei vielen VCL-Projekten dieser Zeit: Datenmodell, Geometrie, Zeichenoperationen, View-Operationen und UI-Adapter sind weitgehend separierbar. Fuer eine Wiederbelebung unter Qt ist das ein guter Ausgangspunkt.

Der wichtigste Befund: Der portierbare CAD-Kern ist nicht die VCL-Komponente selbst, sondern die Kombination aus `sclib`, `comp/mathlib`, `comp/vecobjects`, `comp/main`, `comp/operations`, `comp/lib`, Teilen von `comp/fontengine` und der abstrakten Grafik-Schnittstelle `comp/gengine/vec_graphic_engine.*`. Die eigentlichen Qt-Neuimplementierungen sollten an den alten Adapter-Grenzen entstehen: `comp/vcl`, `vec_graphic_engine_screen`, Cursor/Resource-Loading und Clipboard/Windows-spezifische Utility-Funktionen.

## Gefundene Struktur

Es gibt zwei relevante Quellbaeume:

- `borland_ved/src`: vollstaendigerer Arbeitsbaum mit Testanwendung `vedtest`, `wps`-Integration, Ressourcen und Quelltexten.
- `borland_ved/scved`: daraus gebaute/packagierte Bibliotheksvariante mit Borland-Buildartefakten, `obj`, `bpl_lib` und eigenen Projektdateien.

Empfehlung: `borland_ved/src` als primaere Quelle fuer die Portierung verwenden. `scved` ist wertvoll als Build-Historie und Vergleichsbasis, wirkt aber eher wie die ausgelieferte Komponenten-/Package-Variante.

Weitere Altlasten:

- `zz_vss`: Visual-SourceSafe-Datenbestand. Historisch interessant, fuer eine moderne Portierung nicht direkt benoetigt.
- `*.obj`, `*.tds`, `*.bpl`, `*.lib`, `*.bpi`, `*.map`, `*.res`: alte Buildartefakte.
- `*.bpr`, `*.bpk`, `*.bpg`, `*.dsk`: Borland C++Builder 4 Projekt-/Package-Dateien.
- `*.dfPackage`, `*.wmf`: Together/Diagramm-/Package-Metadaten.

## Groesse und Inhalt

Grobe Zaehlung im gesamten `borland_ved`-Baum:

- 158 `cpp`-Dateien
- 161 `h`-Dateien
- 107 `bmp`-Dateien
- 64 `cur`-Dateien
- 74 `obj`-Dateien
- 40 `dfPackage`-Dateien
- 3 `zip`-Dateien

Der CAD-relevante Kern unter `borland_ved/src/comp` plus `borland_ved/src/sclib` umfasst grob 44.700 Zeilen C++/Header. Die Testanwendung `vedtest` ist zusaetzlich gross und enthaelt viele UI-Details, ist aber als Referenz fuer erwartetes Verhalten sehr wertvoll.

## Modulkarte

| Bereich | Pfad | Rolle | Qt-Portierbarkeit |
|---|---|---|---|
| Basisbibliothek | `src/sclib` | Arrays, Pointer-Arrays, Streams, Factory, Files, Trace, Utils, Messages | Mittel. Viele Teile sind portierbar, aber Windows-Includes in `sc_files`, `sc_memory`, `sc_messages`, `sc_metrics`, `sc_trace`, `sc_utils`. |
| Geometrie | `src/comp/mathlib` | Punkte, Linien, Rechtecke, Ellipsen, Kurven, 2D-Koordinatensysteme, B-Spline/Bezier-Berechnungen | Hoch. Fast reines C/C++, einzelne alte Header wie `values.h` muessen ersetzt werden. |
| CAD-Objekte | `src/comp/vecobjects` | Objektmodell: Linie, Ellipse, Polygon, PolygonArea, Bezier, B-Spline, Text, FrameText, Gruppe, PolyCurve | Hoch bis mittel. Gute OO-Struktur, aber altes Streaming und GraphicEngine-Abhaengigkeit. |
| Kernmodell | `src/comp/main` | `TDVecModel`, `TDVecEdit`, `TDVecEditCad`, View-/Model-Interfaces, Clipboard-Daten, Operation-Manager | Mittel bis hoch. UI-abstrahiert, aber Clipboard und einzelne Typen/Calling-Conventions muessen bereinigt werden. |
| Operationen | `src/comp/operations` | Interaktive CAD-Befehle: create/modify/zoom; Maus-/Keyboard-Events als virtuelle Events | Hoch. Gute Grenze fuer Qt-Input-Mapping. Borland-Keyword `__fastcall` stoert moderne Compiler. |
| Grafikabstraktion | `src/comp/gengine` | Abstrakte `TDGraphicEngine`; GDI-Screen-/Printer-/Engrave-Implementierungen | Interface hoch, Implementierungen niedrig. `TDGraphicEngineQt` sollte neu entstehen. |
| Fonts | `src/comp/fontengine` | Vektor-Fonts, Glyphs, Font-Manager, Defaultfont `.vfn` | Mittel. Fontdaten/Glyphen scheinen wertvoll; Manager enthaelt Windows-Abhaengigkeiten. |
| VCL Adapter | `src/comp/vcl` | `TSCVecView`, `TSCVecModel`, `TSCVecController`, CursorManager | Niedrig. Das ist zu ersetzen durch Qt-Adapter, nicht zu portieren. |
| Test-App | `src/vedtest` | Alte VCL-Demo/Testanwendung mit Toolbars, Menus, zwei Views, Operation-Dialogen | Nicht portieren 1:1. Als funktionale Referenz verwenden. |
| WPS | `src/wps` | Dokument-/WPS-Integration | Spaeter bewerten. Nicht kritisch fuer ersten CAD-Kern. |

## Architektur des CAD-Kerns

### Datenmodell

`TDVecModel` in `src/comp/main/vec_model.h` ist der zentrale Container fuer CAD-Objekte. Es verwaltet:

- Objektliste `TDVecObjList* mpObjects`
- Selektion, Deselektion und Selektion in Bereich
- Gruppieren und Aufloesen von Gruppen
- Loeschen, Verschieben, Rotieren, Farbsetzung und Lock-Flags fuer selektierte Objekte
- Modellgrenzen (`TopLeftArea`, `BottomRightArea`)
- Message-/Callback-Bruecke ueber `TDVecModelInterfaceBase`

Das Modell ist fachlich wertvoll, aber besitzt manuelle Ownership ueber rohe Pointer. Fuer die erste Portierung sollte man diese Semantik nicht sofort modernisieren, sondern zuerst kompilierbar isolieren und mit Tests absichern.

### Objektmodell

`TDVecObject` ist die polymorphe Basisklasse. Wichtige virtuelle Operationen:

- `Draw(TDGraphicEngine*, bool)`
- `DrawNodes(TDGraphicEngine*)`
- `AppendTo(TDMatPolyCurves*)`
- Hit-Testing: `PointOnObject`, `PointOnNode`, `PointOnFrameNode`
- Transformationen: `MoveObject`, `MoveNode`, `Rotate`, `TransformToPoint`, `TransformToOrigin`, `ToScale`
- Geometrieabfragen: `GetFrame`, `GetMidpoint`
- Streaming: Konstruktor aus `TDSimpleInStream`, `WriteToStream`, `CreateFromStream`

Objekttypen laut `vec_maindef.h`:

- `VECOBJ_LIN`
- `VECOBJ_ELL`
- `VECOBJ_POL`
- `VECOBJ_POLAREA`
- `VECOBJ_BSPLINE`
- `VECOBJ_BZC`
- `VECOBJ_TEX`
- `VECOBJ_FRAMETEXT`
- `VECOBJ_GRP`
- `VECOBJ_POLYCURVE`
- `VECOBJ_POLYCURVEAREA`

Das ist eine brauchbare CAD-Domain-Schicht. Besonders positiv ist, dass Geometrieoperationen nicht in der VCL-View stecken.

### Interaktive Operationen

Die Operationen folgen einem Command/State-Muster:

- `TDViewOperation` als abstrakte Basisklasse
- `TDVOCreate` fuer Erzeugungsbefehle
- `TDVOModify` fuer Modifikationsbefehle
- `TDViewOperationManager` als aktiver Dispatcher

Operationen bekommen nur abstrahierte Eingaben:

- `OPMouseDown`
- `OPMouseUp`
- `OPMouseMove`
- `OPKeyDown`
- `OPKeyUp`

Damit ist ein Qt-Adapter gut machbar: `QMouseEvent` und `QKeyEvent` koennen auf `TDOPVirtMouseButton`, `TDOPVirtKey` und `TDOPVirtKeyState` gemappt werden.

Erfasste Operationstypen:

- Create: Linie, Kreise ueber mehrere Varianten, Ellipsen, Polygon/Smartline, Bezier, Rechtecke, B-Spline, PolyCurve, Text, FrameText, RoundRect.
- Modify: Select, Delete, Move Object, Move Node, Move B-Spline Control Point, Modify Curve Attribute, Insert/Delete Node, Rotate, Select+Move, Scale, Round Node, Text Modify, Insert Objects.
- Navigation: Zoom In/Out.

### View/Edit-Schicht

`TDVecEditCad` ist bereits keine VCL-Klasse mehr. Der Header dokumentiert sogar: "V2.00 18.08.00 Umwandlung von VCL nach normale Klasse". Diese Klasse verteilt Maus-/Keyboard-Events an den Operation-Manager, verwaltet bis zu 8 Views und setzt Cursor ueber `TDVecViewInterfaceBase`.

Das ist fuer Qt wichtig: Ein neuer `QVedWidget` muss wahrscheinlich nicht die ganze alte VCL-View nachbauen, sondern kann:

1. Eine `TDVecViewInterfaceTemplate<QVedWidget>`-aehnliche Bruecke bereitstellen.
2. Einen `TDGraphicEngineQt` liefern.
3. Qt Events in die `TDVecEditCad`/`TDViewOperationManager`-Welt weiterreichen.
4. Cursorwechsel aus `TDVecEditCad::UseCursor` in `QWidget::setCursor` umsetzen.

## Ressourcen: Icons, Nodes, Cursor

Die Ressourcensituation passt zur beschriebenen MicroStation-orientierten Bedienung.

Wichtige Pfade:

- Cursor: `src/comp/resource/cur/*.cur`
- Node-Bitmaps: `src/comp/resource/nodes/*.bmp` und `src/comp/resource/nodes.bmp`
- Toolbar-Icons: `src/vedtest/resource/toolbar/*.bmp`
- Vektorfont: `src/comp/resource/font/wps_default.vfn`
- Resource-Script: `src/comp/resource/ved_res.rc`

`ved_res.rc` bindet Cursor fuer konkrete Operationsphasen, z.B. `create_line_1.cur`, `create_line_2.cur`, `create_circle_1.cur`, `create_circle_2.cur`, `select_object.cur`, `zoom.cur`, `delete_object.cur`. Das bestaetigt, dass der Cursorwechsel nicht nur Dekoration ist, sondern Teil der Bedienlogik.

Fuer Qt:

- `.bmp` kann Qt direkt laden, sollte mittelfristig aber nach PNG konvertiert werden.
- `.cur` kann unter Windows teilweise funktionieren, plattformuebergreifend ist PNG/XPM mit Hotspot-Metadaten besser.
- Der alte Resource-ID-Layer sollte durch eine kleine Cursor-/Icon-Registry ersetzt werden, die `TDVecViewCursor` auf Qt-Ressourcen mapped.

## Borland-/Windows-/VCL-Abhaengigkeiten

### Harte VCL-Abhaengigkeit

Stark betroffen:

- `src/comp/vcl/vcl_vec_view.*`
- `src/comp/vcl/vcl_vec_model.*`
- `src/comp/vcl/vcl_vec_controller.*`
- `src/vedtest/main.*`

Typische Marker:

- `#include <vcl.h>`
- `SysUtils.hpp`, `Controls.hpp`, `Classes.hpp`, `Forms.hpp`
- `TComponent`, `TForm`, `TObject`, `TNotifyEvent`
- `__published`, `__property`, `__closure`, `PACKAGE`, `__declspec(delphiclass)`
- `__fastcall`

Diese Dateien sollten nicht direkt portiert werden. Sinnvoller ist ein Qt-Adapter mit gleicher fachlicher Rolle.

### Windows/GDI-Abhaengigkeit

Betroffen:

- `src/comp/gengine/vec_graphic_engine_screen.*`
- `src/comp/gengine/vec_graphic_engine_printer.*`
- `src/comp/vcl/vec_cursormanager.*`
- `src/comp/main/vec_clipdata.*`
- Teile von `src/sclib`
- Teile von `src/comp/fontengine/vec_fontmanager.*`

Typische Marker:

- `windows.h`, `commctrl.h`, `shlobj.h`
- `HDC`, `HPEN`, `HIMAGELIST`, `COLORREF`, `RGB`
- Windows Clipboard
- Resource-Loading aus `.rc`

Fuer Qt ist der wichtigste Ersatz `QPainter` statt GDI. Clipboard wird `QClipboard` oder ein eigenes internes Serialisierungsformat.

### Alte C++/Compiler-Themen

Zu erwarten:

- `__fastcall` in vielen Operationen und VCL-nahen Headern
- Multi-character constants wie `'line'`, `'grup'`, `'elip'` fuer Stream-Factory-IDs
- Header wie `<values.h>` und `<malloc.h>`
- Manuelle Speicherverwaltung mit rohen Pointern
- Alte Include-Pfade ohne Namespace-/Projektstruktur
- Moegliche 32-bit-Annahmen bei `long`, `unsigned long`, Pointergroessen und Streamformat
- Encoding: deutsche Kommentare sind sichtbar nicht UTF-8-sauber importiert

## Streaming/Persistenz

Der Bestand hat ein eigenes Streaming-System:

- `TDStreamable`
- `TDSimpleInStream`
- `TDSimpleOutStream`
- `TDDynamicFactory`
- Factory-Registrierung ueber Vier-Zeichen-IDs: `'line'`, `'grup'`, `'elip'`, `'plyg'`, `'plga'`, `'bezr'`, `'bspl'`, `'plyc'`, `'vtxt'`, `'ftxt'`, `'plca'`, `'vfnt'`, `'vgly'`, `'vchr'`, `'vclp'`

Das ist fuer alte Dateien wichtig. Empfehlung:

1. Streaming zuerst unveraendert kompilierbar halten.
2. Kleine Roundtrip-Tests fuer Modell/Objekte schreiben.
3. Erst danach entscheiden, ob ein Qt-natives Format (`QDataStream`) oder ein explizites neues Format eingefuehrt wird.

Ein Risiko ist die binaere Kompatibilitaet: `long`, Endianness, Alignment und Multi-character constants koennen zwischen Borland C++Builder 4 und modernen Compilern abweichen. Das sollte separat getestet werden, bevor alte Projektdaten als kompatibel angenommen werden.

## Build-Historie

Die Borland-Dateien zeigen C++Builder 4:

- `VERSION = BCB.04.04`
- `USERDEFINES = __DANAK_WINDOWS`
- `SYSDEFINES = NO_STRICT`

`src/vedtest/vedtest.bpr` baut eine VCL-Testanwendung mit:

- `sclib`
- `comp/fontengine`
- `comp/gengine`
- `comp/lib`
- `comp/main`
- `comp/mathlib`
- `comp/operations`
- `comp/vcl`
- `comp/vecobjects`
- `wps`
- externen alten Komponenten `TB97` und `splitctl`

`scved/scvedlib.bpr` baut offenbar `scvedlib.lib` aus dem Kern plus GDI/VCL-nahen Teilen.

`scved/scved.bpk` baut ein Borland Package `scved.bpl`, das nur die VCL-Komponenten plus `scvedlib.lib` einbindet.

Das bestaetigt eine alte Zweiteilung:

1. Kern als statische Library.
2. VCL-Komponenten-Package als UI-/IDE-Integration.

Genau diese Zweiteilung sollte fuer Qt wieder aufgenommen werden:

1. `ved_core` als plattformnaher C++-Kern.
2. `ved_qt` als Qt-Adapter/Widget.

## Ist-Zustand nach Portierungsreife

### Sofort wertvoll

- Geometrie (`mathlib`)
- CAD-Objekte (`vecobjects`)
- Operationen (`operations`)
- Modell/Editor-Logik (`main`, besonders `TDVecModel`, `TDVecEditCad`, `TDViewOperationManager`)
- Resource-Sammlung fuer Cursor/Icons als UX-Referenz
- `vedtest` als Verhalten-/Feature-Katalog

### Muss isoliert oder ersetzt werden

- `comp/vcl`: durch Qt-Adapter ersetzen.
- `vec_graphic_engine_screen`: durch `TDGraphicEngineQt` ersetzen.
- `vec_graphic_engine_printer`: spaeter durch Qt-Print/PDF-Backend ersetzen.
- `vec_cursormanager`: durch Qt-Cursor-Registry ersetzen.
- `vec_clipdata`: Clipboard-Zugriff entkoppeln.
- Windows-spezifische Teile in `sclib`: portable Alternativen oder Qt/Core-Wrapper.

### Nicht zuerst anfassen

- Vollstaendige Modernisierung auf `std::vector`, `std::unique_ptr`, Exceptions usw.
- 1:1 Nachbau der VCL-Testanwendung.
- Vollstaendige Konvertierung aller Ressourcen vor dem ersten lauffaehigen Kern.
- WPS-Integration.

## Empfohlene Portierungsstrategie

### Phase 0: Code einfrieren und Zielbaum anlegen

Ziel: ohne Verhalten zu aendern eine moderne Build-Struktur schaffen.

- `borland_ved/src` als historische Quelle belassen.
- Neuen Zielbereich anlegen, z.B. `src/ved_core` und `src/ved_qt`, oder zunaechst CMake-Ziele, die direkt aus `borland_ved/src` bauen.
- Buildartefakte und VSS-Daten nicht in den aktiven Build aufnehmen.
- Einen Compiler-Kompatibilitaetsheader einfuehren, z.B. fuer `__fastcall`, `PACKAGE`, Borland-Makros.

### Phase 1: Kern kompilierbar machen

Minimalziel:

- `sclib` ohne harte Windows-Funktionen oder mit Stub/Adapter
- `mathlib`
- `lib`
- `vecobjects`
- `main` ohne Clipboard
- `operations`
- abstrakte `TDGraphicEngine`

Noch nicht bauen:

- `comp/vcl`
- `vedtest`
- `vec_graphic_engine_screen`
- `vec_graphic_engine_printer`

Erwartete erste Compilerblocker:

- `values.h`
- `windows.h` in Querschnittsdateien
- `__fastcall`
- alte Borland-Typen/Makros
- Include-Pfad-Konflikte
- Multi-character constants / Factory-IDs

### Phase 2: Headless Tests

Bevor Qt-UI gebaut wird:

- Linie erzeugen, in Modell einfuegen, Frame pruefen.
- Polygon/Ellipse erzeugen, Hit-Testing pruefen.
- Selektion, Move, Rotate, Delete testen.
- Stream Roundtrip fuer einzelne Objekte und Modell.
- Operation-Manager mit synthetischen Maus-Events testen, z.B. `VOC_LINE`.

Diese Tests sichern, dass die Portierung nicht nur kompiliert, sondern die alte CAD-Logik erhalten bleibt.

### Phase 3: Qt-Grafikbackend

Neue Klasse:

- `TDGraphicEngineQt : public TDGraphicEngine`

Mapping:

- `DrawLine` -> `QPainter::drawLine`
- `DrawEllipse` -> `QPainter::drawEllipse`
- `DrawRectangle` -> `QPainter::drawRect`
- `DrawPolygon` -> `QPainter::drawPolygon`
- `DrawNode` -> kleine Pixmap/Primitive fuer Nodes
- Farben `TDRgbColor` -> `QColor`
- Koordinatentransformation aus alter Engine entweder uebernehmen oder mit Qt transformieren

Wichtig: Die alte `TDGraphicEngineScreen` enthaelt nicht nur GDI-Zeichnung, sondern auch Zoom, Pan, DPI, Margins, ViewRange, WorkSpaceRange und Hit-Test-Toleranzen. Diese Semantik muss in `TDGraphicEngineQt` bewusst nachgebildet werden.

### Phase 4: Qt-Widget

Neue UI-Schicht:

- `QVedWidget : public QWidget`
- haelt `TDVecModel`, `TDVecEditCad`, `TDViewOperationManager` oder nutzt einen Controller-Wrapper
- leitet `paintEvent` an `TDVecEditCad::PaintContentOnView`
- mapped Maus-/Keyboard-Events
- setzt Cursor ueber `TDVecViewCursor`
- verwaltet Zoom/Pan/Grid

Die alte VCL-View `TSCVecView` ist dafuer Referenz, aber nicht Zielcode.

### Phase 5: Ressourcen und Bedienkonzept

- Cursor-Mapping `TDVecViewCursor -> QCursor`
- Toolbar-Icons aus `vedtest/resource/toolbar` pruefen und ggf. nach PNG konvertieren
- Node-Grafiken aus `comp/resource/nodes`
- Defaultfont `.vfn` laden und Roundtrip testen

## Risiken

1. Binaerformat: Alte Dateien koennen wegen `long`, Alignment und Borland-Multi-character constants inkompatibel sein.
2. Ownership: Viele rohe Pointer und manuelle Delete-Pfade. Refactoring ohne Tests ist riskant.
3. GraphicEngine-Semantik: Zoom/Pan/Toleranz steckt teilweise in der GDI-Implementierung, nicht nur im abstrakten Interface.
4. Windows-Helfer in `sclib`: Einige Basisfunktionen ziehen Windows-Header in eigentlich portable Schichten.
5. Vektorfont/Text: Text ist meist schwieriger als Linien/Polygone, weil Fontmetriken und Glyphenpfade passen muessen.
6. Encoding: Quelltexte/Kommentare sind vermutlich Windows-1252/ANSI; vor groesserer Bearbeitung Encoding-Strategie festlegen.

## Konkreter naechster Schritt

Der pragmatischste erste technische Schritt ist ein kleiner CMake-/Qt-unabhaengiger `ved_core`-Build, der nur diese Gruppen kompiliert:

- `src/sclib`
- `src/comp/mathlib`
- `src/comp/lib`
- `src/comp/vecobjects`
- `src/comp/main` ohne `vec_clipdata` oder mit Clipboard-Stubs
- `src/comp/operations`
- `src/comp/gengine/vec_graphic_engine.cpp`
- `src/comp/fontengine/vec_font.cpp` zunaechst ohne Windows-Fontmanager, falls moeglich

Danach sofort ein Headless-Test fuer `TDVecModel + TDVecLine + TDViewOperationManager`. Wenn das steht, lohnt sich der Qt-Adapter. Ohne diesen Zwischenschritt wuerde die Qt-Portierung zu schnell UI-Probleme mit Kernproblemen vermischen.

## Erste Zielarchitektur

```text
ved_core
  sclib/
  mathlib/
  vecobjects/
  model/
  operations/
  stream/
  graphic_engine_interface/

ved_qt
  TDGraphicEngineQt
  QVedWidget
  QtCursorRegistry
  QtClipboardAdapter
  QtToolModel / Commands

ved_app
  kleine Qt-Testanwendung
  Toolbar fuer wichtigste Operationen
  zwei Views optional spaeter
```

## Gesamtbewertung

Der Codebestand ist alt, aber fachlich gut strukturiert. Die kritischste Arbeit ist nicht, CAD-Logik neu zu schreiben, sondern die alten Adapter-Schichten sauber abzuschneiden. Der Kern scheint eine realistische Chance zu haben, unter modernem C++ und Qt wieder lauffaehig zu werden, wenn zuerst ein kleiner, testbarer Core-Build entsteht und die VCL/GDI-Schichten bewusst ersetzt statt mechanisch portiert werden.
