# Story: BSpline-Text Performance

Datum: 2026-05-18
Status: offen

## Kontext

Wenn mehrere BSpline-Kurven oder viel Text mit komplexen TrueType-Fonts im Dokument
liegen, wird die Qt-App sehr langsam bis unbedienbar. Die Ursache liegt wahrscheinlich
nicht zuerst im Qt-Painter, sondern in wiederholter Core-Geometrieerzeugung und
Tessellation.

`TDVecBSPLine` berechnet die Kurvenpunkte aktuell sehr haeufig neu:

- `Draw()` ruft bei jedem Paint `ComputeCurve()`.
- `HitTest()` ruft bei jeder Trefferpruefung `ComputeCurve()`.
- `GetFrameMin()` ruft ebenfalls `ComputeCurve()`.
- Viele Mausbewegungen koennen dadurch fuer jedes BSpline-Objekt erneut Kurvenpunkte
  und Basisfunktionen berechnen.

Bei vielen BSplines ist das besonders teuer, weil Paint, Auswahl, Hit-Test und Preview
oft in kurzen Abstaenden laufen.

Text verschaerft dasselbe Problem:

- `TDVecText` besteht aus vielen Glyphen.
- Glyphen bestehen aus `TDVecPolyCurve`.
- komplexe TrueType-Fonts liefern viele Konturen und viele Kurvensegmente.
- viel Text multipliziert diese Kosten pro Paint, Hit-Test und Geometrie-Update.

Deshalb soll diese Story nicht nur BSpline, sondern explizit auch Text optimieren.

## Beobachtete Hotspots

### 1. Kurve wird pro Draw neu berechnet

Datei:

- `src/ved_core/vecobjects/vec_bspline.cpp`

Aktueller Pfad:

- `TDVecBSPLine::Draw(...)`
- ruft immer `ComputeCurve()`
- danach `pGE->DrawPolygon(&curve_, bOutLine)`

Problem:

- Auch unveraenderte BSplines werden bei jedem Repaint komplett neu tesselliert.
- Das ist fuer statische Vektorobjekte unnoetig.

### 2. Hit-Test berechnet ebenfalls neu

Aktueller Pfad:

- `TDVecBSPLine::HitTest(...)`
- `TDVecBSPLine::HitTest(TDGraphicEngine*, ...)`

Problem:

- Hit-Test passiert oft bei MouseMove.
- Wenn jede Mausbewegung pro BSpline eine neue Kurvenberechnung ausloest, skaliert die
  Laufzeit sehr schlecht mit Objektanzahl.

### 3. `ComputeBasisFuns()` allokiert pro Kurvenpunkt

Aktueller Pfad:

- `TDVecBSPLine::ComputeCurve()`
- `TDVecBSPLine::GetCurvePoint(...)`
- `TDVecBSPLine::ComputeBasisFuns(...)`

Problem:

- `ComputeBasisFuns()` erzeugt pro Aufruf mehrere lokale `std::vector<double>`.
- Bei vielen Kurvenpunkten entstehen sehr viele kleine Heap-Allokationen.
- Das ist ein starker Performance-Verlust ohne fachlichen Nutzen.

### 4. Qt-Pfad erzeugt jedes Paint ein neues `QPolygonF`

Datei:

- `src/ved_qt/gengine/vec_graphic_engine_qt.cpp`

Aktueller Pfad:

- `TDGraphicEngineQt::DrawPolygon(...)`
- erzeugt jedes Mal ein neues `QPolygonF`
- transformiert jeden mathematischen Punkt erneut in Screen-Koordinaten

Problem:

- Dieser Punkt ist real, aber zweitrangig.
- Zuerst sollte die Core-Kurve selbst gecacht werden, weil das weniger invasiv ist und
  auch Hit-Test/Operationen verbessert.

### 5. PolyCurve/QSpline-Tessellation betrifft auch Textglyphen

Datei:

- `src/ved_core/vecobjects/vec_polycurve.cpp`

Aktueller Pfad:

- `TDVecPolyCurve::Draw(...)`
- zerlegt `CPT_PRIME_QSPLINE` bei jedem Draw in Zwischenpunkte
- `TDVecPolyCurve::HitTest(...)` erzeugt ebenfalls Draw-Punkte ueber `polyCurveDrawPoints(...)`

Problem:

- TrueType-Konvertierung erzeugt viele Glyph-Konturen als PolyCurves.
- Jeder Text besteht aus vielen dieser PolyCurves.
- Bei komplexen Fonts und viel Text wird dieselbe Geometrie zu oft neu aufgebaut.

### 6. Textobjekte koennen viele Glyphen/PolyCurves enthalten

Datei:

- `src/ved_core/vecobjects/vec_text.cpp`

Problem:

- Textparameter wie Font, Textinhalt, Scale, Winkel, Incline, Spacing und Alignment
  bestimmen die Glyph-Geometrie.
- Solange diese Parameter unveraendert sind, darf die transformierte Textgeometrie nicht
  bei jedem Paint neu erzeugt werden.
- Bei komplexen TrueType-Fonts ist jede unnoetige Neuinitialisierung teuer.

### 7. TrueType-Font-Glyphen muessen fontweit wiederverwendet werden

Dateien:

- `src/app/QtVecFontProviders.cpp`
- `src/ved_core/fontengine/vec_font_manager.*`
- `src/ved_core/fontengine/vec_font.*`

Problem:

- Systemfonts werden in VED-Vektorfonts konvertiert.
- Diese Konvertierung darf nicht objektweise oder paintweise passieren.
- Glyph-Outlines eines Fonts muessen im geladenen `TDVecFont` wiederverwendet werden.

## Ziel

BSplines und Text sollen bei normaler Bedienung mit mehreren Objekten fluessig bleiben:

- keine komplette Kurven-Neuberechnung pro Paint, solange sich das Objekt nicht geaendert hat.
- keine Heap-Allokationen pro Kurvenpunkt in `ComputeBasisFuns()`.
- Hit-Test soll vorhandene Kurvenpunkte wiederverwenden.
- Textgeometrie soll nur neu aufgebaut werden, wenn Text/Font/Attribute geaendert wurden.
- PolyCurve-QSpline-Tessellation soll fuer Textglyphen und normale PolyCurves gecacht werden.
- TrueType-Glyphen sollen fontweit lazy geladen und danach wiederverwendet werden.
- spaeter optional adaptive Zeichenaufloesung je Zoomstufe.

## Vorgeschlagene Umsetzung

### Schritt 1: Dirty-Cache fuer BSpline-Kurve

`TDVecBSPLine` bekommt einen Dirty-Mechanismus:

```cpp
mutable bool curveDirty_ = true;
```

Neue interne Methode:

```cpp
void EnsureCurveComputed() const;
void MarkCurveDirty();
```

Prinzip:

- `Draw()` ruft `EnsureCurveComputed()` statt direkt `ComputeCurve()`.
- `HitTest()` ruft `EnsureCurveComputed()`.
- `GetFrameMin()` ruft `EnsureCurveComputed()`.
- `ComputeCurve()` rechnet nur, wenn `curveDirty_ == true`.
- Nach erfolgreicher Berechnung wird `curveDirty_ = false`.

Alle Methoden, die Kontrollpunkte, Degree oder Resolution veraendern, muessen den Cache
invalidieren:

- `MoveBy`
- `MoveNode`
- `ToScale`
- `Rotate`
- `TransformToPoint`
- `TransformToOrigin`
- `MoveControle`
- `SetDegree`
- `SetResolution`
- `InsertPoint`
- `AppendPoint`
- `RemovePoint`
- `ClearPoints`

Wichtig:

- Das ist fachlich neutral.
- Es aendert nicht die Kurvenform.
- Es vermeidet nur doppelte Arbeit.

### Schritt 2: Keine Heap-Allokationen in `ComputeBasisFuns()`

`ComputeBasisFuns()` soll nicht pro Kurvenpunkt neue Vektoren erzeugen.

Moegliche Loesungen:

- kleine wiederverwendbare `mutable std::vector<double>` Member fuer `left`, `right`, `basis`.
- oder Stack-Array, falls wir Degree sinnvoll begrenzen.

Pragmatische erste Variante:

- Member `basisLeft_`, `basisRight_`, `basisWork_` einfuehren.
- vor Nutzung auf `degree_ + 2` groesse setzen.
- Inhalte ueberschreiben.

Damit bleiben variable Degree-Werte moeglich, aber die meisten Allokationen fallen weg.

### Schritt 3: Reserve und stabile Kurvenpunktzahl

`ComputeCurve()` kennt die ungefaehre Punktzahl:

- Startpunkt
- `resolution_ + 1` Samples
- Endpunkt

Vor dem Befuellen:

```cpp
curve_.clear();
curve_.reserve(resolution_ + 3);
```

Damit werden Re-Allokationen im Kurvenpunktvektor reduziert.

### Schritt 4: PolyCurve Draw-Point-Cache

`TDVecPolyCurve` bekommt einen Cache fuer die gezeichnete/tessellierte Punktliste.

Prinzip:

- `Draw()` baut QSpline-Zwischenpunkte nur bei Dirty neu auf.
- `HitTest()` verwendet dieselbe gecachte Punktliste.
- Linien-Only-PolyCurves koennen direkt die Originalpunkte verwenden oder einen sehr
  einfachen Cache bekommen.

Dirty setzen bei:

- `MoveBy`
- `MoveNode`
- `ToScale`
- `Rotate`
- `TransformToPoint`
- `TransformToOrigin`
- `SetResolution`
- `ChangePointType`
- `InsertPoint`
- `AppendPoint`
- `RemovePoint`
- `ClearPoints`

Nutzen:

- normale PolyCurves profitieren.
- TrueType-Textglyphen profitieren besonders, weil sie viele PolyCurves enthalten.

### Schritt 5: Text-Geometrie-Cache als Muss

`TDVecText` und `TDVecFrameText` sollen ihre erzeugte Glyph-Geometrie nur neu aufbauen,
wenn sich textrelevante Parameter aendern.

Dirty setzen bei:

- Textinhalt
- Fontname/Font-ID
- Fontpointer
- X-/Y-Scale
- Winkel
- Incline
- Zeilenabstand
- Zeichenabstand
- Resolution
- horizontaler Justification
- vertikaler Ausrichtung
- Vertical/Underline
- FrameText-Rechteck
- Scale-with-frame
- Move/Scale/Rotate/Node-Move, falls dadurch Glyph-Geometrie oder Frame abhaengt

Prinzip:

- `EnsureGlyphsInitialized()` statt implizit mehrfacher Initialisierung.
- `Draw()`, `GetFrame()`, `HitTest()`, `DrawNodes()` duerfen nur sicherstellen, dass der
  Cache aktuell ist.
- Parameter-Setter und Operationen markieren den Cache dirty.
- Bereits erzeugte Glyph-PolyCurves bleiben erhalten, solange keine Parameteraenderung vorliegt.

Wichtig:

- Das ist fuer komplexe TrueType-Fonts zwingend.
- Ohne diesen Schritt bleibt viel Text mit komplexen Fonts praktisch unbedienbar.
- Der Cache muss beim Wechsel zwischen VFN/Latin-1 und TrueType/UTF-8-Shaper korrekt
  invalidiert werden.

### Schritt 6: Font-/Glyph-Level Cache pruefen

Der FontManager/Provider soll sicherstellen:

- ein Systemfont wird nur einmal pro Font-ID konvertiert.
- einzelne Glyphen-Outlines werden nicht mehrfach aus FreeType gelesen, wenn derselbe
  Font mehrfach im Dokument benutzt wird.
- HarfBuzz-Shaping wird pro Textobjekt/Run nur neu ausgefuehrt, wenn Text oder Font
  geaendert wurde.

Falls aktuell schon ein ganzer `TDVecFont` pro Font-ID im Speicher bleibt, muss das
explizit abgesichert und getestet werden.

### Schritt 7: Adaptive Zeichenaufloesung spaeter pruefen

Optional nach Schritt 1-6:

- Beim Zeichnen kann eine zoomabhaengig reduzierte Kurve benutzt werden.
- Beim starken Herauszoomen braucht eine BSpline nicht 50+ Segmente, wenn viele Punkte auf
  denselben Pixel fallen.
- Hit-Test sollte dabei vorsichtig bleiben, damit Auswahl nicht ungenau wird.

Moegliche Regel:

- interne geometrische Kurve bleibt in Originalaufloesung gecacht.
- Qt-Zeichnen darf Screen-Punkte ausduennen, wenn aufeinanderfolgende Punkte im gleichen
  oder benachbarten Pixel landen.

### Schritt 8: Qt-Polyline-Cache nur falls noetig

Falls nach Core-Cache immer noch Engpaesse bleiben:

- pro View/Zoom einen Screen-Polyline-Cache pruefen.
- `TDGraphicEngineQt::DrawPolygon(...)` koennte redundant gleiche Punktlisten vermeiden.

Dieser Schritt ist groesser und UI-spezifischer. Deshalb erst spaeter.

## Akzeptanzkriterien

- Mehrere BSpline-Objekte im Dokument machen die App nicht mehr extrem langsam.
- Viel Text mit komplexen TrueType-Fonts bleibt bedienbar.
- Repaint unveraenderter BSplines berechnet die Kurve nicht erneut.
- Hit-Test unveraenderter BSplines nutzt gecachte Kurvenpunkte.
- Repaint unveraenderter Textobjekte erzeugt Glyph-Geometrie nicht erneut.
- Repaint unveraenderter PolyCurves tesselliert QSpline-Punkte nicht erneut.
- Aenderungen an Kontrollpunkten, Degree, Resolution, Move, Scale und Rotate invalidieren
  den Cache korrekt.
- Aenderungen an Textinhalt, Font und Textattributen invalidieren den Textcache korrekt.
- Systemfont-Konvertierung bleibt fontweit gecacht/lazy und passiert nicht paintweise.
- Bestehende BSpline-Tests bleiben erfolgreich.
- Bestehende Text- und Font-Tests bleiben erfolgreich.
- Keine fachliche Aenderung der Kurvenform.
- Keine fachliche Aenderung der Textdarstellung.

## Tests

Sinnvolle Test-Ergaenzungen:

- BSpline-Objekt mit mehreren Kontrollpunkten zeichnen/hit-testen und sicherstellen, dass
  wiederholte Aufrufe gleiche Ergebnisse liefern.
- Nach `MoveControle()` aendert sich die Hit-Test-/Frame-Geometrie.
- Nach `SetResolution()` wird die Kurvenpunktzahl aktualisiert.
- Nach `MoveBy`, `ToScale`, `Rotate` bleiben Draw-/Hit-Test-Ergebnisse plausibel.
- Textobjekt mit komplexem TrueType-Font wiederholt zeichnen/hit-testen und gleiche
  Ergebnisse erhalten.
- Nach Text-/Font-/Scale-/Winkel-Aenderung wird Textgeometrie neu erzeugt.
- Nach unveraendertem Repaint wird Textgeometrie nicht neu erzeugt.
- PolyCurve mit `CPT_PRIME_QSPLINE` wiederholt zeichnen/hit-testen und Cache wiederverwenden.

Zusaetzlich sinnvoll:

- kleiner Performance-Test oder Debug-Zaehler fuer `ComputeCurve()` im Testpfad, falls das
  ohne Produktionscode-Verschmutzung machbar ist.
- analoger Debug-Zaehler fuer Text-Glyph-Initialisierung und PolyCurve-Tessellation.

## Verifikation nach Umsetzung

Mindestens ausfuehren:

```text
cmake --build cmake-build-debug --target ved_core_curve_object_tests
cmake --build cmake-build-debug --target ved_core_polygon_polycurve_operation_tests
cmake --build cmake-build-debug --target ved_core_text_tests
cmake --build cmake-build-debug --target ved_core_vec_font_tests
cmake --build cmake-build-debug --target ved_qt_app
ctest --test-dir cmake-build-debug --output-on-failure
```

## Hinweise

- Der erste Fix sollte im Core passieren, nicht im Qt-Painter.
- Das alte VED-Verhalten wird dadurch nicht geaendert.
- Text-Optimierung ist kein Nice-to-have; komplexe TrueType-Fonts mit viel Text muessen
  explizit Teil dieser Story sein.
- Eine spaetere adaptive Darstellung muss vorsichtig eingefuehrt werden, damit Auswahl und
  Bearbeitung nicht ungenau wirken.
