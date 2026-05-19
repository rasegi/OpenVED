Port Step 3.001 - Plan Portierung_Phase_3: SCLIB und Text auf Basis wps_default.vfn
Datum: 2026-05-16

Ziel
====

Phase 3 portiert Text in die Qt-App.

Wichtige Entscheidung:

- Keine TrueType-Konvertierung in diesem Schritt.
- Kein Plugin fuer Text/Fonts in diesem Schritt.
- Basis ist zuerst nur `borland_ved/src/comp/resource/font/wps_default.vfn`.
- Die alte `sclib` wird fachlich 1:1 uebernommen.
- Eine Modernisierung auf STL/modernes C++ passiert erst spaeter in Phase 4 oder 5.

Analyse: Warum SCLIB jetzt noetig ist
=====================================

`wps_default.vfn` ist kein freies Fontformat und kein Qt-Font. Die Datei enthaelt nach einem
1024-Byte-Header einen alten VED-Stream mit serialisierten Objekten:

- `vfnt` fuer `TDVecFont`
- `vchr` fuer `TDVecCharacter`
- `vgly` fuer `TDVecGlyph`
- `plyc` fuer `TDVecPolyCurve`

Die Borland-Text- und Fontklassen haengen direkt an alten SCLIB-Typen:

- `TDStreamable`
- `TDSimpleInStream`
- `TDStream`
- `TDDynamicFactory`
- `TDObjPtrArray`
- `TDObjArray`
- `TDArray`
- `ObjPtrArrayReadFromStream`
- Utility-Funktionen wie `StringCopy`, `StringSize`, `MemoryAlloc`, `MemoryReAlloc`

Im aktuellen Qt-Core gibt es davon praktisch noch nichts. In
`src/ved_core/main/vec_maindef.h` existiert nur eine Forward-Deklaration:

- `template<class T>class TDObjPtrArray;`
- `typedef TDObjPtrArray<TDVecObject> TDVecObjList;`

Damit ist klar: Text/VFN kann nicht sauber portiert werden, ohne vorher den benoetigten
SCLIB-Teil in den Qt-Core zu holen.

Analyse: SCLIB-Dateien
======================

Borland-SCLIB liegt unter:

- `borland_ved/src/sclib`

Vorhandene Module:

- `sc_array`
- `sc_array_stream`
- `sc_dynamic_factory`
- `sc_easy_array`
- `sc_files`
- `sc_memory`
- `sc_messages`
- `sc_metrics`
- `sc_object_array`
- `sc_pointer_array`
- `sc_simple_stream`
- `sc_stream`
- `sc_streamable`
- `sc_trace`
- `sc_utils`

Fuer Text/VFN direkt benoetigter Kern:

- `sc_array.h/.cpp`
- `sc_object_array.h/.cpp`
- `sc_pointer_array.h/.cpp`
- `sc_array_stream.h/.cpp`
- `sc_dynamic_factory.h/.cpp`
- `sc_streamable.h/.cpp`
- `sc_stream.h/.cpp`
- `sc_simple_stream.h/.cpp`
- `sc_memory.h/.cpp`
- `sc_utils.h/.cpp`

Nicht zuerst mitnehmen, solange Text/VFN sie nicht braucht:

- `sc_files.h/.cpp`
- `sc_messages.h/.cpp`
- `sc_metrics.h/.cpp`
- `sc_trace.h/.cpp`
- `sc_easy_array.h/.cpp`, falls keine direkte Text-Abhaengigkeit auftaucht

Begruendung:

- `sc_files`, `sc_messages`, `sc_metrics` und `sc_trace` enthalten Windows- oder alte
  Plattformpfade.
- `sc_easy_array` enthaelt selbst keine Windows-Abhaengigkeit. Es ist ein Template-Wrapper
  ueber `TDObjArray` und wird nur deshalb nicht zuerst mitgenommen, solange Text/VFN es
  nicht direkt braucht.
- Text/VFN kann aus Qt-Resource-Bytes in einen `TDSimpleInStream` geladen werden.
- Dafuer braucht Phase 3 keinen alten Windows-Datei-, Message-, Metrics- oder Trace-Layer.

Analyse: Windows- und Plattformabhaengigkeiten
==============================================

Gefundene Windows-Pfade in SCLIB:

- `sc_trace.cpp`
  - `windows.h`
  - `HANDLE`
  - Windows-Mailslot/Trace-Pfade
  - fuer Text/VFN nicht noetig
- `sc_messages.h/.cpp`
  - `windows.h`
  - `HWND`
  - alte Message-/Resource-Anzeige
  - fuer Text/VFN nicht noetig
- `sc_files.h/.cpp`
  - `windows.h`
  - `HANDLE`
  - File-Mapping/Datei-API
  - fuer Text/VFN nicht noetig, wenn Qt die Resource-Bytes liefert
- `sc_metrics.cpp`
  - `windows.h`
  - `GetLocaleInfo`, `LOCALE_USER_DEFAULT`, `LOCALE_SDECIMAL`
  - `MulDiv`
  - fachlich keine Device- oder Font-Metrics, sondern Masseinheitenumrechnung:
    mm/inch, Dezimalseparator, String-Konvertierung interner 1/1000-mm-Werte
  - fuer Text/VFN nicht noetig
- `sc_utils.h/.cpp`
  - hat Windows- und Linux-Zweige
  - enthaelt aber auch benoetigte String-/Utility-Funktionen
  - muss fuer macOS/Qt mit einem kleinen Kompatibilitaetspfad gebaut werden
- `sc_memory.cpp`
  - Windows-Zweig mit Heap-API
  - Linux-Zweig mit `malloc_usable_size`
  - macOS hat stattdessen `malloc_size`
  - fuer Arrays/Streams noetig, also muss hier ein minimaler Plattform-Fix her

Gefundene Windows-Pfade ausserhalb SCLIB:

- `borland_ved/src/comp/fontengine/vec_fontmanager.h/.cpp`
  - `windows.h`
  - `HDC`, `LOGFONT`, `HFONT`
  - `GetGlyphOutline`
  - `TTPOLYGONHEADER`, `TTPOLYCURVE`

Entscheidung:

- `vec_fontmanager` wird in Phase 3 nicht 1:1 komplett uebernommen.
- Die Windows/GDI-TrueType-Konvertierung bleibt draussen.
- Fuer Phase 3 reicht ein kleiner Default-Font-Ladepfad fuer `wps_default.vfn`.
- Die eigentlichen Fontdatenklassen `vec_font.h/.cpp` sind dagegen wichtig und werden portiert.

Analyse: Syntax- und Binary-Risiken
===================================

1. `long`-Breite im Streamformat

`sc_stream.cpp` liest und schreibt alte `signed long` und `unsigned long` direkt mit
`sizeof(signed long)` bzw. `sizeof(unsigned long)`.

Im Borland-Win32-Ursprung war `long` 4 Byte. Auf macOS/Linux 64-bit ist `long` 8 Byte.

Das ist der wichtigste technische Risikopunkt, weil das alte VFN-Binaerformat sonst falsch
gelesen wird. Fachlich darf der Stream nicht modernisiert werden; noetig ist ein
Kompatibilitaets-Fix fuer das alte 32-bit-Format.

Moegliche saubere Loesung fuer Phase 3:

- SCLIB fachlich 1:1 uebernehmen.
- Nur die Plattform-/Compilerkompatibilitaet kapseln.
- Stream-Primitive fuer altes VED-Format explizit auf 32-bit pruefen.
- Dazu sofort ein kleiner VFN-Ladetest, damit der Fehler nicht erst bei Text sichtbar wird.

2. Alte Header und Compiler-Unterschiede

Gefunden:

- `sc_dynamic_factory.h` nutzt `#include <typeinfo.h>`.

Auf modernen Compilern kann dafuer `#include <typeinfo>` noetig sein. Das ist ein
Syntax-/Compiler-Fix, keine fachliche Aenderung.

3. Plattformmakros

Viele SCLIB-Dateien erwarten:

- `__DANAK_WINDOWS`
- oder `__DANAK_LINUX`

Fuer macOS/Qt sollte kein Windows-Pfad aktiviert werden. Wahrscheinlich ist ein kleiner
Kompatibilitaets-Layer sinnvoll, der fuer die alten Quellen einen POSIX-aehnlichen Pfad
bereitstellt, ohne SCLIB fachlich umzubauen.

Plan Step 3.001: SCLIB-Basis 1:1 in Qt-Core bringen
===================================================

Umfang:

- Benoetigten SCLIB-Kern in den Qt-Core kopieren.
- Fachliche Logik nicht modernisieren.
- Keine STL-Ersetzung.
- Keine Umbenennung der alten Klassen als fachliche Portierung.
- Nur notwendige Syntax-, Include- und Plattform-Fixes.

Kandidaten fuer ersten Import:

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

Bewusst nicht im ersten Import:

- `sc_files`
- `sc_messages`
- `sc_metrics`
- `sc_trace`

Akzeptanz fuer Step 3.001:

- SCLIB-Kern baut als eigener Core-Teil.
- Keine Qt-Abhaengigkeit in SCLIB.
- Factory kann FourCC-Klassen registrieren.
- Stream kann einfache Werte lesen.
- Binary-Kompatibilitaet fuer alte 32-bit-Streamwerte ist geklaert und getestet.

Plan Step 3.002: Fontengine und wps_default.vfn laden
=====================================================

Umfang:

- `TDVecFont`, `TDVecCharacter`, `TDVecGlyph` aus `vec_font.h/.cpp` portieren.
- `TDVecPolyCurve` als Glyph-Kontur ueber vorhandenen Core nutzen.
- Factory-Registrierungen ergaenzen:
  - `vfnt`
  - `vchr`
  - `vgly`
  - `plyc`
- `wps_default.vfn` als Qt-Resource oder App-Resource einbinden.
- Beim Laden die ersten 1024 Bytes ueberspringen.
- Danach per `TDSimpleInStream` und `TDDynamicFactory` `TDVecFont` lesen.

Nicht-Ziel:

- Kein Windows-`TDFontManager::CreateVecFontFromSystemTT`.
- Kein `GetGlyphOutline`.
- Kein Plugin.
- Kein TrueType.

Akzeptanz fuer Step 3.002:

- Default-Font wird aus `wps_default.vfn` geladen.
- Font enthaelt plausible Characters/Glyphs/PolyCurves.
- Ein Test liest mindestens den Default-Font und prueft bekannte Grunddaten.
- Fehler beim Fontladen sind sichtbar und enden nicht in spaeteren Null-Pointern.

Plan Step 3.003: Textobjekte in den Core portieren
==================================================

Umfang:

- `TDVecText` portieren.
- `TDVecFrameText` portieren.
- Alte Textparameter fachlich erhalten:
  - Textinhalt
  - Position
  - Winkel
  - Skalierung
  - Schraegstellung/Incline
  - Zeichenabstand
  - Zeilenabstand
  - Unterstreichung
  - vertikaler Text
  - horizontale/vertikale Ausrichtung
  - Fontname/Fontreferenz
  - Aufloesung/Resolution
- Factory-Registrierungen ergaenzen:
  - `vtxt`
  - `ftxt`

Wichtig:

- Text muss mit dem in Step 3.002 geladenen `wps_default.vfn` funktionieren.
- Zeichnen bleibt VED-Vektorgeometrie, nicht Qt-Text.
- Lock-Regeln aus Phase 2 muessen auch fuer Textobjekte gelten.
- Node-Darstellung und selektierte Textobjekte muessen wie die anderen Objekte in den
  existierenden Qt-Mechanismus passen.

Akzeptanz fuer Step 3.003:

- Textobjekt kann im Core erzeugt werden.
- FrameText kann im Core erzeugt werden.
- Draw-/Hit-/Bounds-Pfade laufen ohne Absturz.
- Serialisierte alte Textobjekte koennen ueber Factory rekonstruiert werden, soweit die
  alten Streamdaten verfuegbar sind.

Plan Step 3.004: Textoperationen und minimale UI-Anbindung
==========================================================

Umfang:

- Text-Erzeugungsoperation portieren:
  - `VOC_VECTEXT`
- FrameText-Erzeugung portieren:
  - `VOC_VECFRAMETEXT_EMPTY`
- Text-Modifikation portieren:
  - `VOM_VECTEXT`
- Toolbar/Menu/Command-Anbindung fuer Text herstellen.
- Minimaler Parameterdialog oder vorhandene Parameterflaeche nur mit Default-Font.

Nicht-Ziel:

- Keine Fontauswahl aus Systemfonts.
- Kein Plugin.
- Kein TrueType-Import.
- Kein Font-Cache.
- Kein kompletter Font-Editor.

Akzeptanz fuer Step 3.004:

- Benutzer kann mit `wps_default.vfn` Text erzeugen.
- Benutzer kann vorhandenen Text bearbeiten.
- Benutzer kann FrameText erzeugen.
- Loeschen, Verschieben, Skalieren, Locks und Selektion verhalten sich wie bei den anderen
  portierten Objekten.
- Gesamttests und App-Build laufen.

Offene Punkte fuer die Umsetzung
================================

- Vor Code-Import entscheiden, wo der SCLIB-Kern im Qt-Projekt liegt, vermutlich unter
  `src/ved_core/sclib`.
- Klaeren, ob die alten Namen exakt bleiben oder nur der Pfad angepasst wird.
- `long`-Binary-Kompatibilitaet vor dem ersten echten VFN-Ladetest loesen.
- macOS-Speicherfunktion fuer `sc_memory.cpp` loesen (`malloc_size` statt
  `malloc_usable_size`).
- Windows-only SCLIB-Module nur dann anfassen, wenn eine echte Text-Abhaengigkeit auftaucht.
- `sc_metrics` spaeter bei Bedarf ueber Qt-/Kompatibilitaetsaequivalente portieren:
  - `GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, ...)` entspricht fuer unseren Zweck
    `QLocale::system().decimalPoint()`.
  - `MulDiv(a, b, c)` hat kein direktes Qt-Aequivalent, kann aber als kleine
    64-bit-sichere Kompatibilitaetsfunktion nachgebildet werden.
  - Dabei das alte Rundungs-/Truncation-Verhalten pruefen, falls gespeicherte Werte oder UI
    exakt Borland-kompatibel bleiben muessen.
- `vec_fontmanager` nicht komplett uebernehmen, solange der Windows/GDI-Teil nicht gebraucht
  wird.

Zusammenfassung
===============

Der sinnvolle Phase-3-Weg ist:

1. SCLIB-Kern 1:1 fachlich portieren und nur build-/plattformfaehig machen.
2. `wps_default.vfn` ueber den alten Stream/Factory-Weg laden.
3. `TDVecText` und `TDVecFrameText` auf dieser Fontbasis portieren.
4. Textoperationen und minimale UI anbinden.

Damit bleibt Phase 3 eng am alten VED-Format und vermeidet bewusst Plugin-, TrueType- und
STL-Umbauten. Diese Umbauten gehoeren spaeter in Phase 4 oder 5.
