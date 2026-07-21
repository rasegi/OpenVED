# Story: Text-Portierung Phase 3

Datum: 2026-05-17
Status: geschlossen

## Kontext

Text in VED ist kein nativer Qt-Text. Das alte Borland-VED erzeugt und zeichnet Text als
VED-Vektorgeometrie:

- `TDVecText` und `TDVecFrameText` sind normale VED-Objekte.
- Buchstaben werden aus `TDVecGlyph` aufgebaut.
- Glyphen bestehen aus `TDVecPolyCurve`.
- VED-Fonts werden als `.vfn` ueber SCLIB/`TDStreamable` serialisiert.
- Der alte Default-Font liegt als `wps_default.vfn` vor.

Ziel der Phase war eine VED-treue Textbasis im Qt-Port: erst der alte VFN-Pfad, danach
die alten Textoperationen, danach System-TrueType-Fonts als in-memory VED-Fonts.

## Ziel

Phase 3 sollte Text im Qt-Port wieder benutzbar machen:

- SCLIB-Kern fuer VFN/Text uebernehmen.
- `wps_default.vfn` laden.
- VED-Fontengine portieren.
- `TDVecText` und `TDVecFrameText` portieren.
- Text ueber die alten `VOC`-/`VOM`-Operationen erzeugen und bearbeiten.
- Text im Qt-UI ueber ein Dock testbar machen.
- System-TrueType-Fonts als VED-Vektorfonts in Speicher konvertieren.
- Arabisch/Farsi ueber HarfBuzz shaping korrekt fuer reine RTL-Texte darstellen.
- Font-Auswahl im Text-Dock normal bedienbar halten.

## Umsetzung

### SCLIB fuer Text/VFN

Importiert wurde der fuer Text/VFN benoetigte SCLIB-Kern unter `src/ved_core/sclib`.
Die Logik wurde fachlich nicht modernisiert.

Importiert:

- `sc_array`
- `sc_object_array`
- `sc_pointer_array`
- `sc_array_stream`
- `sc_dynamic_factory`
- `sc_streamable`
- `sc_stream`
- `sc_simple_stream`
- `sc_memory`
- `sc_utils`

Wichtige Portierungsanpassungen:

- alte Borland-Streamwerte `signed long`/`unsigned long` werden weiter als 32-bit-Werte gelesen und geschrieben.
- `TDStreamable`/Factory bleibt die Basis fuer VFN/Text.
- macOS-Speicherabfrage nutzt `malloc_size`.
- Windows-only SCLIB-Module wurden nicht importiert, solange sie fuer Text/VFN nicht gebraucht werden.

### VFN-Fontengine

Portiert wurde die alte VED-Fontstruktur:

- `TDVecFont`
- `TDVecCharacter`
- `TDVecGlyph`

Ergaenzt:

- `RegisterVecFontStreamTypes()`
- `LoadVecFontFromMemory(...)`
- `VecStreamFourCC(...)`

Registrierte Stream-Typen:

- `vfnt` -> `TDVecFont`
- `vchr` -> `TDVecCharacter`
- `vgly` -> `TDVecGlyph`
- `plyc` -> `TDVecPolyCurve`

`wps_default.vfn` liegt jetzt unter `src/app/resources/font/wps_default.vfn` und wird als Qt-Resource eingebunden.
Die Datei wird ab Byte `1024` als alter VED-Stream gelesen.

### Core-Textobjekte

Neu im Core:

- `TDVecText`
- `TDVecFrameText`

Portierte/aktivierte Textparameter:

- Textinhalt
- Ursprungspunkt
- X-/Y-Skalierung
- Winkel
- Neigung/Incline
- Zeilenabstand
- Zeichenabstand
- Vertikal-Flag
- Underline-Flag
- horizontale Ausrichtung
- vertikale Ausrichtung
- Resolution
- Fontname
- Vektorfont-Pointer
- FrameText-Rechteck und Scale-with-frame

Umgesetzt:

- Glyphen werden aus dem aktiven VED-Font erzeugt.
- Zeilenumbruch wird verarbeitet.
- Skalierung, Rotation und Incline werden auf Glyph-Geometrie angewendet.
- `Draw(...)` zeichnet Text als VED-PolyCurves.
- `GetFrame()`, `MoveBy`, `Rotate`, `ToScale`, `TransformToPoint`, `TransformToOrigin` funktionieren.
- Locks werden respektiert.
- Text und FrameText sind ueber SCLIB-Streams serialisierbar.

### Text-Nodes

Die Node-Bearbeitung wurde nach Borland-Verhalten korrigiert:

- `TDVecText::PointOnNode` erkennt die 8 Frame-Nodes des Text-Frames.
- `TDVecText::MoveNode` skaliert Text ueber diese Frame-Nodes.
- `TDVecFrameText` erkennt und bewegt seine Nodes anhand seines Rechtecks.
- `VOM_SELECT_MOVE_NODE_OBJECT` und `VOM_MOVE_NODE` koennen Text/FrameText bearbeiten.

### Text-Dock und Bedienung

Das Qt-Text-Dock ersetzt fuer Phase 3 die alte Dialogbox ausreichend fuer Tests.

Vorhandene Bedienfelder:

- mehrzeilige Texteingabe
- Font-Auswahl
- X-Scale
- Y-Scale
- Winkel
- Neigung
- Zeilenabstand
- Zeichenabstand
- Aufloesung
- horizontale Ausrichtung
- vertikale Ausrichtung
- Vertical
- Underline
- Scale-with-frame fuer FrameText

Der gruene Haken schreibt geaenderte Attribute in die aktive Operation.

### Alte Textoperationen

Die zuerst vorhandenen direkten MainWindow-Erzeugungspfade wurden wieder entfernt.
Text laeuft jetzt ueber portierte Core-Operationen.

Portiert:

- `TDVOCVecText`
- `TDVOCVecTextExtVar`
- `TDVOCVecFrameTextEmpty`
- `TDVOCVecFrameTextEmptyExtVar`
- `TDVOMVecText`
- `TDVOMVecTextExtVar`

Aktiv im `TDViewOperationManager`:

- `VOC_VECTEXT`
- `VOC_VECFRAMETEXT_EMPTY`
- `VOM_VECTEXT`

Verhalten:

- `cmdVOCVecText` startet die alte Text-Erzeugung.
- Text haengt als Preview am Cursor.
- Linksklick platziert den Text an der Mauskoordinate.
- `cmdVOCVecFrameTextEmpty` zeichnet zuerst eine leere Box, wie im alten VED.
- Der Textinhalt fuer FrameText wird danach separat ueber `cmdVOMVecText` bearbeitet.
- `cmdVOMVecText` bearbeitet selektierte Text-/FrameText-Objekte ueber ExtVar und `SendUpdateToParrentOP()`.

### Rechtsklick bei Dialog-/Dock-Operationen

Rechtsklick beendet Dialog-/Dock-Operationen zentral, aber nicht Operationen, bei denen
Rechtsklick fachlich Accept/Finish bedeutet.

Zentraler Cancel bleibt fuer:

- Curve-Attribute-Dock
- RoundRectangle-Dock
- Rotate-Dock
- Text-Dock
- FrameText-Box-Erzeugung

Ausgenommen:

- `VOC_POLYCURVE`
- `VOC_BSPLINE_CONTROLPOINT`

Diese Operationen bekommen Rechtsklick weiter durchgereicht, weil Borland dort Rechtsklick
als Akzeptieren/Beenden verwendet.

### Crash-Fix FrameText-Preview

`TDVOCVecFrameTextEmpty::OPMouseMove()` erzeugte die Preview zunaechst lokal und gab einen
Raw-Pointer an `TDVecEditCad::TmpAppend()`. Nach Rueckkehr aus `OPMouseMove()` war das Objekt
geloescht, der Paint-Pfad konnte aber noch den Pointer benutzen.

Korrektur:

- Die Preview lebt jetzt als Member `mpTmpObject` der Operation.
- Das entspricht dem Muster anderer Rectangle-VOCs.
- Der Crash beim ersten Klick/Drag nach Aktivierung von `cmdVOCVecFrameTextEmpty` ist damit behoben.

### FontManager und Provider

Eingefuehrt:

- `TDFontManager`
- `IVecFontProvider`
- `TDVecFontDescriptor`

Der Core bleibt Qt-frei. Der FontManager delegiert Fontquellen an Provider:

- Built-in VFN Provider fuer `wps_default.vfn`
- Qt/Systemfont Provider ausserhalb des Core fuer TrueType-Fonts

Der Default-Font `wps_default.vfn` ist ohne Plugin verfuegbar.
Systemfonts werden lazy geladen/konvertiert und im Speicher gehalten.

### TrueType-Konvertierung

System-TrueType-Fonts werden ausserhalb des Core ueber Qt/FreeType/HarfBuzz bereitgestellt.

Umgesetzt:

- Systemfont-Liste aus Qt/System-Fontdaten.
- Versteckte Dot-Fonts werden aus der UI-Liste gefiltert.
- Nicht skalierbare Fonts werden ignoriert.
- FreeType liest Glyph-Outlines.
- TrueType-Linien werden zu `CPT_PRIME_LINE`.
- Quadratic Splines werden zu `CPT_PRIME_QSPLINE`.
- Cubic-Kurven werden pragmatisch segmentiert.
- Konvertierte Fonts bleiben im Speicher.

Wichtig:

- Das ist kein Plugin mehr in Phase 3, sondern eine direkte App-seitige Provider-Klasse.
- Der Core kennt nur das Provider-Interface.
- Persistentes Speichern konvertierter Fonts als `.vfn` bleibt spaeterer Arbeitspunkt.

### UTF-8, Latin-1 und wps_default.vfn

Fuer `wps_default.vfn` wird Text byteweise wie im alten VED behandelt.
Umlaute funktionieren ueber Latin-1-Codes:

- `0xE4` ae
- `0xF6` oe
- `0xFC` ue
- `0xDF` ss

Fuer TrueType-Fonts wurde der Textpfad auf UTF-8 erweitert.
`TDVecText` kann einen optionalen Shaper verwenden. Wenn ein Shaper fuer den aktiven Font
vorhanden ist, liefert dieser fertig positionierte Glyphen.

### HarfBuzz und RTL/Farsi

Fuer TrueType-Fonts wurde HarfBuzz-Shaping eingebunden.

Umgesetzt:

- Arabic/Farsi werden ueber HarfBuzz geformt.
- Reine RTL-Texte werden in korrekter visueller Reihenfolge erzeugt.
- Glyph-IDs und Positionen kommen aus HarfBuzz.
- FreeType liefert danach die passenden Outline-Glyphen.
- Gemischte LTR/RTL-Zeilen wurden pragmatisch unterstuetzt, ohne vollstaendigen Unicode-Bidi-Layout-Anspruch.

Getestete Problemfaelle aus der Entwicklung:

- persische Woerter wurden anfangs gespiegelt dargestellt.
- einzelne arabische Buchstaben wurden anfangs nicht verbunden.
- reine Farsi/Arabisch-Texte wurden danach korrekt geformt.
- gemischte Texte wie lateinisch plus Farsi wurden stabiler, bleiben aber nicht als vollstaendige Textlayout-Engine definiert.

### Font-Auswahl

Die Font-Auswahl im Text-Dock zeigt sichtbare Prefixe:

- `VC: ...` fuer VED/VFN-Fonts
- `TT: ...` fuer TrueType/Systemfonts

Die Tastatur-Suche ignoriert den sichtbaren Prefix.
Beispiel: Eingabe `Ar` springt zu `TT: Arial...`.

Wichtige Korrektur:

- Die ComboBox bleibt nicht editierbar und sieht normal aus.
- Der Suchtext wird gegen eine separate Search-Role gesucht.
- `FontComboBox::eventFilter()` ruft kein `view()` mehr auf.
- Dadurch ist die Rekursion aus `QComboBox::view()` beseitigt, die einen Stack Overflow beim App-Start ausloesen konnte.

## Geaenderte Hauptbereiche

- `CMakeLists.txt`
- `src/app/MainWindow.h`
- `src/app/MainWindow.cpp`
- `src/app/MainWindowTextDock.cpp`
- `src/app/MainWindowToolbars.cpp`
- `src/app/QVedWidget.h`
- `src/app/QVedWidget.cpp`
- `src/app/QtVecFontProviders.h`
- `src/app/QtVecFontProviders.cpp`
- `src/app/resources/font/wps_default.vfn`
- `src/ved_core/sclib/*`
- `src/ved_core/fontengine/vec_font.h`
- `src/ved_core/fontengine/vec_font.cpp`
- `src/ved_core/fontengine/vec_font_manager.h`
- `src/ved_core/fontengine/vec_font_manager.cpp`
- `src/ved_core/vecobjects/vec_text.h`
- `src/ved_core/vecobjects/vec_text.cpp`
- `src/ved_core/operations/voc_vectext.*`
- `src/ved_core/operations/voc_vecframetext_empty.*`
- `src/ved_core/operations/vom_vectext.*`
- `src/ved_core/operations/vop_manager.*`
- `tests/ved_core_sclib_tests.cpp`
- `tests/ved_core_vec_font_tests.cpp`
- `tests/ved_core_text_tests.cpp`

## Akzeptanzkriterien

- `wps_default.vfn` wird aus Qt-Resources geladen.
- Text wird als VED-Vektorgeometrie gezeichnet.
- `TDVecText` und `TDVecFrameText` sind Core-Objekte.
- Text kann erzeugt, selektiert, ueber Nodes bearbeitet und geloescht werden.
- `cmdVOCVecText` benutzt die alte Core-Operation.
- `cmdVOCVecFrameTextEmpty` zeichnet zuerst eine leere Box.
- `cmdVOMVecText` bearbeitet selektierte Textobjekte ueber die alte VOM-Operation.
- Umlaute funktionieren mit `wps_default.vfn` ueber Latin-1.
- System-TrueType-Fonts sind in der Font-Auswahl sichtbar.
- TrueType-Fonts werden bei Bedarf in VED-Vektorfonts konvertiert.
- Arabisch/Farsi funktioniert fuer reine RTL-Texte ueber HarfBuzz.
- Font-Auswahl bleibt eine normale ComboBox.
- Tastatursuche in der Font-Auswahl ignoriert `TT:`/`VC:`-Prefixe.
- App startet ohne ComboBox-Rekursion/Stack Overflow.

## Verifikation

Wiederholt ausgefuehrt waehrend der Story:

```text
cmake --build cmake-build-debug --target ved_qt_app
cmake --build cmake-build-debug --target ved_core_sclib_tests
cmake --build cmake-build-debug --target ved_core_vec_font_tests
cmake --build cmake-build-debug --target ved_core_text_tests
ctest --test-dir cmake-build-debug --output-on-failure
```

Letzter Stand:

- `ved_qt_app` baut erfolgreich.
- Gesamte Testsuite erfolgreich: 20/20.
- App-Start per macOS `open cmake-build-debug/ved_qt_app.app` ohne Shell-Startfehler.

## Bewusst offen fuer spaeter

- Persistentes Speichern konvertierter TrueType-Fonts als `.vfn`.
- Vollstaendiger Font-Cache mit Invalidierung.
- Echte Plugin-Auslagerung des TrueType-Font-Converters.
- Vollstaendige Unicode-Bidi-Textlayout-Engine fuer komplexe gemischte Texte.
- Vollstaendige VED-Paritaet fuer alle seltenen Textattribute wie Vertical/Underline, falls in alten Dateien relevant.
- Spaetere Modernisierung von SCLIB erst in einer spaeteren Phase, nicht in Phase 3.

## Referenzen

- `Portierung_Phase_3/port_step_3_001.log`
- `Portierung_Phase_3/port_step_3_002.log`
- `Portierung_Phase_3/port_step_3_003.log`
- `Portierung_Phase_3/port_step_3_004.log`
- `scrapbook/open_stories/story_font_provider_truetype_converter_plugin.md`
