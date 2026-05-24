# Story: BSpline-Text Performance

Datum: 2026-05-18
Status: in Arbeit

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

### Schritt 1: Dirty-Cache fuer BSpline-Kurve — erledigt 2026-05-24

`TDVecBSPLine` hat einen Dirty-Mechanismus bekommen:

- `mutable bool curveDirty_ = true` als Member
- `EnsureCurveComputed()` als const Guard (rechnet nur wenn dirty)
- `MarkCurveDirty()` zum Invalidieren

Umgestellte Leser:

- `Draw()`, `HitTest()` (2 Overloads), `GetFrameMin()` rufen `EnsureCurveComputed()`

Invalidierung durch alle Mutatoren:

- `MoveBy`, `MoveNode`, `ToScale`, `Rotate`, `TransformToPoint`, `TransformToOrigin`
- `MoveControle`, `SetDegree`, `SetResolution`
- `InsertPoint`, `AppendPoint`, `RemovePoint`, `ClearPoints`

Zusaetzlich bereinigt:

- `ToScale()` transformiert `curve_` nicht mehr direkt — Kurve wird lazy aus
  skalierten Kontrollpunkten neu berechnet.
- `TransformToPoint()` / `TransformToOrigin()` verschieben nur noch `controls_`
  und setzen dirty.
- `SetDegree()` und `SetResolution()` invalidieren den Cache jetzt korrekt
  (fehlte vorher).

Alle 21 Tests bestanden. Keine fachliche Aenderung.

### Schritt 2: Keine Heap-Allokationen in `ComputeBasisFuns()` — erledigt 2026-05-24

`ComputeBasisFuns()` erzeugt keine lokalen Vektoren mehr pro Kurvenpunkt.

Umsetzung:

- Member `basisLeft_`, `basisRight_` (mutable) eingefuehrt.
- `basisFuns_` (bereits vorhanden) wird direkt als Arbeitsvektor benutzt
  statt ueber ein separates `n`-Array kopiert zu werden.
- `assign(size, 0.0)` ueberschreibt den Inhalt ohne Allokation, solange
  die Kapazitaet ausreicht (nach erstem Aufruf der Fall).
- `basisWork_` aus der Story-Planung entfaellt, da `basisFuns_` diese Rolle
  direkt uebernimmt.

Damit fallen bei 50 Kurvenpunkten ~150 Heap-Allokationen pro `ComputeCurve()` weg.

### Schritt 3: Reserve und stabile Kurvenpunktzahl — erledigt 2026-05-24

`ComputeCurve()` ruft vor dem Befuellen:

```cpp
curve_.reserve(resolution_ + 3);
```

Damit werden Re-Allokationen im Kurvenpunktvektor bei stabilem `resolution_`-Wert
vollstaendig vermieden.

### Schritt 4: PolyCurve Draw-Point-Cache — erledigt 2026-05-24

`TDVecPolyCurve` hat einen Cache fuer die tessellierte Punktliste bekommen:

- `mutable TDMatPointsArray drawCache_` + `mutable bool drawCacheDirty_ = true`
- `EnsureDrawCacheComputed()` baut die Punktliste via `polyCurveDrawPoints()` nur bei Dirty
- `Draw()` nutzt den Cache und zeichnet via `DrawPolygon` (statt inline Tessellation)
- `HitTest()` (2 Overloads) nutzt denselben Cache statt `polyCurveDrawPoints()` pro Aufruf

Invalidierung durch alle Mutatoren:

- `MoveBy`, `MoveNode`, `ToScale`, `Rotate`, `TransformToPoint`, `TransformToOrigin`
- `SetResolution`, `ChangePointType`
- `InsertPoint`, `AppendPoint`, `RemovePoint`, `ClearPoints`

Nutzen:

- normale PolyCurves profitieren.
- TrueType-Textglyphen profitieren besonders, weil sie viele PolyCurves enthalten.
- Draw und HitTest teilen sich denselben Cache — keine doppelte Tessellation mehr.

Alle 21 Tests bestanden. Keine fachliche Aenderung.

### Schritt 5: Text-Geometrie-Cache — erledigt 2026-05-24

`TDVecText` und `TDVecFrameText` bauen Glyph-Geometrie nur noch bei Parameteraenderung
neu auf.

Umsetzung:

- `mutable bool glyphsDirty_ = true` als Member in `TDVecText`
- `EnsureGlyphsInitialized()` als const Guard (nutzt `const_cast` fuer virtuelles
  `InitializeGlyphs()`)
- `MarkGlyphsDirty()` zum Invalidieren
- `InitializeGlyphs()` setzt `glyphsDirty_ = false` am Anfang

Leser die den Cache sicherstellen:

- `Draw()`, `GetFrame()`, `CountGlyphs()` rufen `EnsureGlyphsInitialized()`
- `HitTest()` und `DrawNodes()` sind ueber `GetFrame()` / `Draw()` abgedeckt

Invalidierung durch Mutatoren:

- `SetText`, `SetVecFontPointer`, `SetJustification`, `SetVerticalAlignment`
- `SetParameter`, `ToScale`, `Rotate`, `Initialize`
- `SetXScale`, `SetYScale`, `SetAngle`, `SetIncline` (fehlte vorher)
- `SetLineSpacing`, `SetCharSpacing` (fehlte vorher)
- `TDVecFrameText`: `SetRectangle`, `ToScale`, `SetParameter`

Beseitigt doppelte Initialisierung:

- `TDVecFrameText::SetParameter` rief `InitializeGlyphs` doppelt auf (einmal via
  `TDVecText::SetParameter`, einmal direkt) — jetzt nur einmal bei erstem Zugriff.
- `TDVecFrameText::ToScale` ebenso.

Alle 21 Tests bestanden. Keine fachliche Aenderung.

### Schritt 6: Font-/Glyph-Level Cache — erledigt 2026-05-24

Pruefung und Absicherung:

- `TDFontManager::GetVecFontExact()` cached geladene Fonts pro Font-ID korrekt.
  Ein Systemfont wird nur einmal konvertiert und danach wiederverwendet. (war bereits
  korrekt, verifiziert)

- `TDQtSystemFontProvider::ShapeText()` hat einen `VecShapingCache` bekommen:
  FreeType-Library, FT_Face und hb_font_t werden pro Font-ID gecacht und bei
  wiederholtem Shaping-Aufruf mit demselben Font wiederverwendet.
  Vorher: FT_Init_FreeType + FT_New_Face + hb_ft_font_create pro ShapeText-Aufruf.
  Nachher: nur beim ersten Aufruf oder Fontwechsel.

- HarfBuzz-Shaping wird durch den Text-Geometrie-Cache (Schritt 5) nur bei
  Parameteraenderung ausgefuehrt, nicht pro Paint.

Alle 21 Tests bestanden.

### Schritt 7: Adaptive Zeichenaufloesung — erledigt 2026-05-24

`TDGraphicEngineQt::DrawPolygon()` duennt beim Zeichnen Punkte aus, die auf denselben
Screen-Pixel fallen.

Umsetzung:

- Beim Aufbau des `QPolygonF` wird jeder Punkt zu Screen-Koordinaten transformiert.
- Falls ein Punkt auf denselben Pixel faellt wie der vorherige, wird er uebersprungen.
- Erster und letzter Punkt werden immer beibehalten (Kurvenendpunkte).
- Hit-Test bleibt bei voller Aufloesung (nutzt gecachte Core-Kurve, nicht Qt-Rendering).
- Bei hohem Zoom sind alle Punkte verschieden — kein Qualitaetsverlust.
- Bei starkem Herauszoomen werden redundante Punkte eliminiert — weniger Zeichenaufwand
  fuer Qt.

Alle 21 Tests bestanden. Keine fachliche Aenderung.

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
