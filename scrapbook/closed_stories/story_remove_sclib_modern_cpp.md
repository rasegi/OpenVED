# Story: SCLIB durch modernes C++ ersetzen

Datum: 2026-05-18
Status: umgesetzt

## Kontext

Der aktuelle Qt-Port enthaelt noch Teile der alten Borland-`sclib` unter
`src/ved_core/sclib`. Diese Bibliothek war fuer Borland/Win32 und spaeter
Linux-Portierung gebaut. Sie bringt alte Plattformschalter, eigene
Speicherverwaltung, eigene String-Helfer, eigene Array-Typen, eigene Streams und
eine alte Runtime-Factory mit.

Das passt nicht zum Ziel des neuen Ports:

- Core-Code soll modernes C++ verwenden.
- Plattformunterschiede sollen nicht ueber alte `__DANAK_WINDOWS` /
  `__DANAK_LINUX`-Pfade in fachlichen Core-Dateien liegen.
- Manuelle Speicherverwaltung soll durch RAII, `std::vector`, `std::unique_ptr`,
  `std::string`, `std::array`, `std::span`, `std::byte` und klare Ownership
  ersetzt werden.
- Das bestehende Verhalten des aktuellen Ports muss erhalten bleiben,
  insbesondere Laden/Schreiben der aktuell unterstuetzten alten VED/VFN-Streams.

Wichtig: Diese Story endet erst, wenn aus dem aktuellen Core nichts mehr von
`sclib` uebrig ist:

- kein `src/ved_core/sclib`
- keine `sc_*.h` Includes
- keine `TDArray`, `TDObjPtrArray`, `TDStream`, `TDSimpleInStream`,
  `TDSimpleOutStream`, `TDStreamable`, `TDDynamicFactory`
- keine `Memory*`- oder `String*`-Hilfsfunktionen aus `sclib`
- keine `__DANAK_WINDOWS` / `__DANAK_LINUX`-Schaltung fuer `sclib`

## Aktueller Stand

Aktuell sind folgende `sclib`-Dateien Teil von `ved_core`:

- `sc_array.*`
- `sc_array_stream.*`
- `sc_dynamic_factory.*`
- `sc_memory.*`
- `sc_object_array.*`
- `sc_pointer_array.*`
- `sc_simple_stream.*`
- `sc_stream.*`
- `sc_streamable.*`
- `sc_utils.*`

Direkte Nutzung ausserhalb von `sclib`:

- `vec_font.cpp/.h`
  - `TDObjPtrArray`
  - `TDStreamable`
  - `TDSimpleInStream`
  - `TDSimpleOutStream`
  - `StringCopy`
- `vec_polycurve.cpp/.h`
  - `TDSimpleInStream`
  - `TDSimpleOutStream`
  - `TDStreamable`
- `vec_object.cpp/.h`
  - `TDStreamable`
  - `TDSimpleInStream`
  - `TDSimpleOutStream`
- `vec_text.cpp/.h`
  - `TDVecGlyphArray`
  - `TDSimpleInStream`
  - `TDSimpleOutStream`
  - `TDStreamable`
  - `StringCopy`
  - `StringSize`
- Tests
  - `ved_core_sclib_tests`
  - `ved_core_text_tests`
  - `ved_core_vec_font_tests`

`sc_utils` ist fachlich kaum noch gebraucht. Die relevanten Verwendungen sind
im Wesentlichen `StringCopy`, `StringLen` und `StringSize`.

Die tiefere Kopplung liegt in Streams, Factory und persistenten Klassen. Diese
Schicht darf nicht nur mechanisch geloescht werden, weil aktuell Font- und
Textobjekte ueber das alte binaere VED/VFN-Format geladen und getestet werden.

## Ziel

Nach Abschluss der drei Iterationen gibt es keine Abhaengigkeit auf `sclib`
mehr. Der aktuelle Funktionsstand bleibt erhalten:

- bestehende Core-Operationen laufen weiter.
- Textobjekte funktionieren weiter.
- Default-/VFN-Font-Laden funktioniert weiter.
- aktuelle Stream-Tests fuer Text, PolyCurve und Font bleiben erhalten oder
  werden durch gleichwertige Tests gegen die neue Serialisierung ersetzt.
- alte 32-bit-Borland-Win32-Streamsemantik fuer `long`-Werte bleibt beim Lesen
  und Schreiben der aktuell unterstuetzten Formate erhalten.

## Nicht-Ziele

- Keine neue Dokumentformat-Version als Voraussetzung fuer diese Story.
- Keine fachliche Neuinterpretation von VED-Geometrie, Textlayout oder Fontdaten.
- Keine Boost-Pflicht. Falls Standard-C++ reicht, wird Standard-C++ verwendet.
- Keine selbstgebauten Ersatz-Wrapper fuer `Memory*`, um die alte Form nur unter
  anderem Namen zu behalten.
- Keine Beibehaltung eines `sclib_compat`-Ordners als Dauerloesung.

## Grundsatzentscheidungen

### Speicher und Buffer

Bei Streams und Arrays werden `MemoryAlloc`, `MemoryReAlloc`, `MemoryFree`,
`MemoryMove`, `MemoryZero` und aehnliche Helfer nicht durch neue Wrapper
ersetzt.

Stattdessen:

- Stream-Buffer: `std::vector<std::byte>` fuer owned write/read buffers.
- Read-only Input: `std::span<const std::byte>` oder Pointer+Size nur an der
  API-Grenze, intern sofort als Span behandeln.
- Schreiben: bounds-gepruefte Append-/Write-Methoden.
- Lesen: bounds-gepruefte Read-Methoden mit explizitem Fehlerstatus.
- Kopieren/Move: `std::memcpy`, `std::memmove`, `std::fill`, `std::copy` direkt
  dort, wo sie fachlich noetig sind.

### Ownership

Pointer-Arrays werden durch RAII ersetzt:

- `TDObjPtrArray<T>` -> `std::vector<std::unique_ptr<T>>`
- Transfer-Funktionen wie `Drop...()` behalten ihre Semantik, werden aber mit
  `std::unique_ptr` intern sauber umgesetzt.
- Copy/Clone-Semantik wird explizit in den betroffenen Klassen implementiert.

### Serialisierung

Die alte Streamable/Factory-Schicht wird nicht 1:1 nachgebaut. Sie wird durch
eine kleine moderne Core-Serialisierung ersetzt:

- `VEDBinaryReader`
- `VEDBinaryWriter`
- `VEDStreamRegistry` oder typbezogene Factory-Funktionen
- feste Typbreiten: `std::uint8_t`, `std::uint16_t`, `std::uint32_t`,
  `std::int32_t`
- klare FourCC-Hilfen fuer alte Signaturen
- keine Abhaengigkeit auf RTTI als zentrale Persistenzlogik, wenn direkte
  Registrierung oder `std::type_index` sauberer ist.

## Iteration 1: Utility- und Memory-Cut

Status: umgesetzt am 2026-05-18

### Ziel

`sc_utils` verschwindet komplett aus dem aktiven Build. `sc_memory` wird nicht
mehr als allgemeine Core-Abhaengigkeit verwendet. Die Windows-Kompilierbarkeit
wird dadurch nicht durch alte Linux/Windows-Zweige blockiert.

### Arbeiten

1. Direkte `sc_utils`-Nutzung entfernen:
   - `StringCopy` in `vec_font.cpp` und `vec_text.cpp` ersetzen.
   - `StringSize` in `vec_text.cpp` ersetzen.
   - `StringLen`, `StringSize`, `StringCopy` in der noch vorhandenen
     Stream-Schicht ersetzen oder auf die neue Iteration-1-Stream-Vorbereitung
     verschieben.

2. C-String-Felder bewusst behandeln:
   - Fuer feste Borland-kompatible Felder wie `char msFontName[30]` vorerst
     stabile, lokale Standard-C++-Logik nutzen.
   - Falls moeglich `std::array<char, 30>` vorbereiten, aber nur wenn die
     Serialisierung dadurch nicht veraendert wird.

3. `sc_utils.cpp/.h` aus `ved_core` entfernen:
   - aus `CMakeLists.txt`
   - aus Includes
   - aus Tests, falls indirekt gebraucht

4. `sc_memory`-Nutzung reduzieren:
   - In neu angefassten Codepfaden keine `Memory*`-Funktionen mehr verwenden.
   - Noch verbleibende Nutzung in `sc_array`/`sc_stream` bleibt nur temporaer
     bis Iteration 2/3.

5. Windows-Build korrigieren:
   - Keine pauschale `__DANAK_LINUX`-Definition fuer Windows.
   - Nach Entfernen von `sc_utils` muss Windows nicht mehr die alten Registry-,
     Shell- und WinAPI-Utility-Pfade kompilieren.

### Abnahmekriterien

- Kein produktiver Include von `sc_utils.h`.
- `sc_utils.cpp/.h` sind nicht mehr Teil von `ved_core`.
- Alle bestehenden Tests laufen auf macOS.
- Windows-Build kommt mindestens ueber den bisherigen `sc_memory.cpp:48`-Fehler
  hinaus.
- Keine neue Wrapper-Bibliothek fuer alte `String*`/`Memory*`-Funktionen.

### Erwartete Rest-SCLIB nach Iteration 1

Noch vorhanden:

- `sc_array.*`
- `sc_array_stream.*`
- `sc_dynamic_factory.*`
- `sc_memory.*`
- `sc_object_array.*`
- `sc_pointer_array.*`
- `sc_simple_stream.*`
- `sc_stream.*`
- `sc_streamable.*`

Entfernt:

- `sc_utils.*`

### Umsetzungsnotiz 2026-05-18

Erledigt:

- `sc_utils.cpp/.h` aus `ved_core` entfernt.
- Produktive Includes von `sc_utils.h` entfernt.
- `StringCopy`, `StringLen`, `StringSize` in den aktiven Nutzern durch
  Standard-C++/C-Standardfunktionen ersetzt:
  - `std::snprintf` fuer feste C-String-Felder.
  - `std::strlen` und `std::memcpy` fuer dynamische Textkopien.
- `sc_stream.cpp` von `sc_utils` und `sc_memory` geloest:
  - `TDStream` wird mit `new (std::nothrow)` / `delete` verwaltet.
  - Stream-Datenkopien laufen ueber `std::memcpy`.
  - Z-String-Laengen laufen ueber `std::strlen`.
- CMake-Plattformdefinition korrigiert:
  - Windows bekommt `__DANAK_WINDOWS`.
  - Nicht-Windows bekommt temporaer weiter `__DANAK_LINUX`.

Verifikation:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 20/20 Tests bestanden.

Bewusst verbleibend fuer Iteration 2/3:

- `sc_utils.cpp/.h` liegen noch als Datei im Ordner, sind aber nicht mehr Teil
  des aktiven Builds. Der komplette Ordner wird erst nach Iteration 3 entfernt.
- `sc_memory` bleibt noch fuer `sc_array` aktiv, bis die Container in
  Iteration 2 durch Standardcontainer ersetzt werden.

## Iteration 2: Container-Cut

Status: umgesetzt am 2026-05-18

### Ziel

Die alten Array- und Pointer-Array-Typen verschwinden aus Font/Text/PolyCurve.
Ownership wird mit Standard-C++ ausgedrueckt.

### Arbeiten

1. Font-Container ersetzen:
   - `TDPolyCurveArray* mpPolyCurves` in `TDVecGlyph`
   - `TDVecGlyphArray*`
   - `TDVecCharacterArray* mpCharacters` in `TDVecFont`

2. Text-Glyph-Container ersetzen:
   - `TDVecGlyphArray* mpGlyphsArray` in `TDVecText`
   - intern `std::vector<std::unique_ptr<TDVecGlyph>>`

3. Oeffentliche Klassenmethoden erhalten:
   - `Insert...`
   - `Append...`
   - `Remove...`
   - `Drop...`
   - `Delete...`
   - `Clear...`
   - `Get...`
   - `Count...`
   - `Compact...`

   Die Methodennamen duerfen vorerst bleiben, damit Fachcode und Tests nicht
   gleichzeitig grossflaechig umgebaut werden muessen. Intern wird aber kein
   `TDObjPtrArray` mehr verwendet.

4. Clone-/Copy-Semantik explizit implementieren:
   - `TDVecGlyph`
   - `TDVecCharacter`
   - `TDVecFont`
   - `TDVecText`
   - `TDVecFrameText`

5. Stream-Array-Helfer entfernen:
   - `sc_array_stream` nicht mehr fuer Font/Text/PolyCurve verwenden.
   - Array-Serialisierung mit Schleifen ueber `std::vector` schreiben.

6. Alte Array-Dateien aus dem Build entfernen:
   - `sc_array.*`
   - `sc_object_array.*`
   - `sc_pointer_array.*`
   - `sc_array_stream.*`

### Abnahmekriterien

- Kein produktiver Include von:
  - `sc_array.h`
  - `sc_object_array.h`
  - `sc_pointer_array.h`
  - `sc_array_stream.h`
- Keine produktive Verwendung von:
  - `TDArray`
  - `TDObjArray`
  - `TDObjPtrArray`
- Font-/Text-Tests laufen unveraendert oder mit gleichwertigen aktualisierten
  Erwartungen.
- Kein neuer Raw-Owner-Pointer fuer Containerbesitz.

### Erwartete Rest-SCLIB nach Iteration 2

Noch vorhanden:

- `sc_dynamic_factory.*`
- `sc_memory.*`, falls nur noch indirekt durch alte Stream-Schicht gebraucht
- `sc_simple_stream.*`
- `sc_stream.*`
- `sc_streamable.*`

Entfernt:

- `sc_utils.*`
- `sc_array.*`
- `sc_object_array.*`
- `sc_pointer_array.*`
- `sc_array_stream.*`

### Umsetzungsnotiz 2026-05-18

Erledigt:

- `TDObjPtrArray` aus Font/Text entfernt.
- `TDVecGlyph` verwendet intern `std::vector<std::unique_ptr<TDVecPolyCurve>>`.
- `TDVecText` verwendet intern `std::vector<std::unique_ptr<TDVecGlyph>>`.
- `TDVecFont` verwendet intern `std::vector<std::unique_ptr<TDVecCharacter>>`.
- `TDVecCharacter` besitzt sein Glyph per `std::unique_ptr<TDVecGlyph>`.
- Die bisherigen public Methoden fuer Insert/Append/Remove/Drop/Delete/Clear/Get/Count/Compact
  bleiben erhalten, damit bestehender Fachcode nicht gleichzeitig umgebaut werden muss.
- Array-Stream-Helfer `ObjPtrArrayReadFromStream` und `ObjPtrArrayWriteToStream`
  wurden in den Fontklassen durch direkte Schleifen ueber `std::vector` ersetzt.
- `TDDynamicFactory` nutzt intern `std::vector` statt `TDObjArray`, damit
  `sc_object_array` nicht mehr als indirekte Restabhaengigkeit gebraucht wird.
- Unbenutzte alte `TDObjPtrArray`-/`TDEasyArray`-Forward-Deklarationen aus
  `vec_maindef.h` entfernt.
- `sc_array.*`, `sc_array_stream.*`, `sc_object_array.*` und `sc_pointer_array.*`
  aus `ved_core` entfernt.

Verifikation:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 20/20 Tests bestanden.

Bewusst verbleibend fuer Iteration 3:

- `sc_memory.*`
- `sc_stream.*`
- `sc_simple_stream.*`
- `sc_streamable.*`
- `sc_dynamic_factory.*`

Diese Dateien bilden noch die alte Stream-/Factory-Schicht und werden erst in
Iteration 3 durch `ved_core/serialization` ersetzt.

## Iteration 3: Stream- und Factory-Cut

### Ziel

Die alte Persistenzschicht wird vollstaendig ersetzt. Danach existiert keine
`sclib`-Abhaengigkeit mehr.

### Arbeiten

1. Neue moderne Binary-Serialisierung einfuehren:
   - `src/ved_core/serialization/ved_binary_reader.*`
   - `src/ved_core/serialization/ved_binary_writer.*`
   - optional `src/ved_core/serialization/ved_stream_registry.*`

2. Reader/Writer-Verhalten festlegen:
   - Little-endian wie aktuelles Borland-Win32-Format.
   - `ReadInt32`, `ReadUInt32`, `ReadUInt16`, `ReadDouble`, `ReadBool`,
     `ReadCString`.
   - `WriteInt32`, `WriteUInt32`, `WriteUInt16`, `WriteDouble`, `WriteBool`,
     `WriteCString`.
   - Fehlerstatus statt stiller Buffer-Overruns.
   - Owned Output Buffer als `std::vector<std::byte>`.
   - Input als `std::span<const std::byte>`.

3. `TDStreamable`-Basisklasse entfernen:
   - `TDVecObject` erbt nicht mehr von `TDStreamable`.
   - `TDVecFont`, `TDVecGlyph`, `TDVecCharacter`, `TDVecPolyCurve`, `TDVecText`,
     `TDVecFrameText` erben nicht mehr von `TDStreamable`.

4. Objekt-Factories ersetzen:
   - alte `TDStreamable::Factory()->Register(...)`-Aufrufe entfernen.
   - FourCC-basierte Lese-/Schreibfunktionen modern abbilden.
   - Fuer bekannte Typen direkte Factory-Funktionen nutzen, z.B.
     `ReadVecObjectByFourCC`, `ReadVecFontObjectByFourCC`.

5. Konstruktoren und Methoden migrieren:
   - alte Konstruktoren mit `TDSimpleInStream*` entfernen oder durch
     `static ReadFrom(VEDBinaryReader&)` ersetzen.
   - alte `WriteToStream(TDSimpleOutStream*)` durch
     `WriteTo(VEDBinaryWriter&) const` ersetzen.

6. Tests migrieren:
   - `ved_core_sclib_tests` entfernen oder durch
     `ved_core_serialization_tests` ersetzen.
   - Text-/Font-/PolyCurve-Roundtrip-Tests auf neue Reader/Writer umstellen.
   - Test mit `wps_default.vfn` bzw. vorhandenen Fontdaten beibehalten, damit
     alte VFN-Kompatibilitaet bewiesen bleibt.

7. Letzte SCLIB-Dateien entfernen:
   - `sc_dynamic_factory.*`
   - `sc_memory.*`
   - `sc_simple_stream.*`
   - `sc_stream.*`
   - `sc_streamable.*`
   - Ordner `src/ved_core/sclib`
   - Include-Pfad `src/ved_core/sclib` aus CMake
   - `__DANAK_*`-Definitionen fuer SCLIB aus CMake

### Umsetzungsnotiz Arbeitspaket 1, 2026-05-18

Erledigt:

- Neue Serialisierungskomponenten angelegt:
  - `src/ved_core/serialization/ved_binary_reader.h/.cpp`
  - `src/ved_core/serialization/ved_binary_writer.h/.cpp`
- `VEDBinaryWriter` schreibt in einen owned `std::vector<std::byte>`.
- `VEDBinaryReader` liest aus `std::span<const std::byte>` oder
  Pointer+Groesse an der API-Grenze.
- Primitive Werte werden explizit little-endian gelesen/geschrieben:
  - `std::int8_t` / `std::uint8_t`
  - `std::int16_t` / `std::uint16_t`
  - `std::int32_t` / `std::uint32_t`
  - `double`
  - `bool`
  - Enum als 32-bit signed Wert
  - C-String mit Nullterminator
  - FourCC ueber `VEDMakeFourCC(...)`
- Reader hat einen klaren Fehlerstatus:
  - `EndOfBuffer`
  - `UnterminatedString`
  - `InvalidArgument`
- Neue Tests `ved_core_serialization_tests` pruefen:
  - alte 32-bit-`long`-Bytefolge fuer `-1234567`: `79 29 ed ff`
  - FourCC `vfnt` als Bytes `v f n t`
  - UInt16-Little-Endian
  - Roundtrip fuer `double`, `bool`, Enum und C-Strings
  - Buffer-Overrun ohne Positionsfortschritt
  - nicht terminierte Strings ohne Positionsfortschritt
  - Raw-Byte-Lesen und Skip
- `ved_core` bindet die neue `serialization`-Schicht und den Include-Pfad ein.

Bewusst noch nicht erledigt:

- Keine Migration der bestehenden `TDSimpleInStream`/`TDSimpleOutStream`-Nutzer
  in Font/Text/PolyCurve/Object. Das folgt in den naechsten Arbeitspaketen.
- Kein `VEDStreamRegistry` angelegt, weil Arbeitspaket 1 noch keine Objektfactory
  migriert. Die Registry wird erst eingefuehrt, wenn die FourCC-Objektleser
  konkret gebraucht werden.

Verifikation:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 21/21 Tests bestanden.

### Umsetzungsnotiz Arbeitspaket 2, 2026-05-18

Erledigt:

- Gemeinsame Formatdefinitionen in
  `src/ved_core/serialization/ved_binary_format.h` ausgelagert:
  - `VEDBinaryError`
  - `VEDMakeFourCC(...)`
- `VEDBinaryReader` und `VEDBinaryWriter` nutzen diesen gemeinsamen Header,
  damit FourCC- und Fehlervertrag nicht an einen Writer-only Header gekoppelt
  sind.
- Der Reader/Writer-Vertrag ist jetzt durch Tests fuer alle alten
  Stream-Primitiven abgesichert:
  - signed/unsigned 8-bit
  - signed/unsigned 16-bit
  - signed/unsigned 32-bit
  - alter Borland-`long` als expliziter 32-bit-Wert
  - `double`
  - `bool`
  - Enum als signed 32-bit
  - C-String
  - FourCC
- Cross-Kompatibilitaet ist im Test belegt:
  - alter `TDSimpleOutStream` schreibt, neuer `VEDBinaryReader` liest.
  - neuer `VEDBinaryWriter` schreibt, alter `TDSimpleInStream` liest.

Bewusst noch nicht erledigt:

- Die Cross-Kompatibilitaet nutzt temporaer noch `sc_simple_stream.h` im Test.
  Das ist Absicherung fuer die Migration, keine neue produktive Abhaengigkeit.
  Der Test wird spaeter entweder entfernt oder auf rein moderne Roundtrips
  umgestellt, wenn die alte Stream-Schicht geloescht wird.
- Noch keine Objekt-/Typmigration. Das folgt ab Arbeitspaket 3 mit
  `TDStreamable` und den FourCC-Factories.

Verifikation:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 21/21 Tests bestanden.

### Umsetzungsnotiz Arbeitspaket 3, 2026-05-18

Erledigt:

- Fachliche TD-Klassen erben nicht mehr von `TDStreamable`:
  - `TDVecObject`
  - `TDVecPolyCurve`
  - `TDVecGlyph`
  - `TDVecCharacter`
  - `TDVecFont`
  - `TDVecText`
  - `TDVecFrameText`
- `sc_streamable.h` ist aus den fachlichen Headern entfernt.
- `CreateFromStream(...)` und alte `TDStreamable::Factory()->Register(...)`
  Aufrufe sind aus Font/Text/PolyCurve entfernt.
- Moderne Serialisierungs-API an den real serialisierten Typen eingefuehrt:
  - `StreamFourCC()`
  - `TypeFourCC()` fuer polymorphe `TDVecObject`-Ableitungen
  - `WriteTo(VEDBinaryWriter&) const`
  - `static ReadFrom(VEDBinaryReader&)`
- `TDVecObject` schreibt/liest seinen gemeinsamen Objektzustand ueber
  `WriteTo(...)` und `ReadObjectStateFrom(...)`.
- `LoadVecFontFromMemory(...)` liest `wps_default.vfn` jetzt ueber
  `VEDBinaryReader` und direkte FourCC-Pruefung statt ueber
  `TDSimpleInStream::ReadObject()`.
- Text-/FrameText-Roundtrip-Tests nutzen jetzt `VEDBinaryWriter` und
  `VEDBinaryReader`.
- `ved_core_serialization_tests` enthaelt jetzt auch einen direkten
  `TDVecPolyCurve`-Roundtrip.

Bewusst noch temporaer:

- Legacy-Konstruktoren und `WriteToStream(TDSimpleOutStream*)` sind noch als
  Payload-Helfer vorhanden, aber ohne `TDStreamable`-Basisklasse und ohne alte
  Factory. Sie bleiben nur bis zum naechsten Arbeitspaket, damit der Umbau
  kontrolliert in kleineren Schritten bleibt.
- `sc_simple_stream` und `sc_streamable` sind noch im aktiven Build, weil der
  isolierte `ved_core_sclib_tests` die alte Bibliothek noch prueft. Fachliche
  TD-Klassen haengen nicht mehr daran.

Verifikation:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 21/21 Tests bestanden.

### Umsetzungsnotiz Arbeitspaket 4, 2026-05-18

Erledigt:

- Temporaere Legacy-Payload-Helfer aus fachlichen Klassen entfernt:
  - Konstruktoren mit `TDSimpleInStream*`
  - `WriteToStream(TDSimpleOutStream*)`
- Alte Stream-Includes aus fachlichen Quellen entfernt:
  - kein `sc_simple_stream.h` mehr in Font/Text/PolyCurve/Object-Code
  - kein `sc_streamable.h` mehr in Font/Text/PolyCurve/Object-Code
- No-op-Registrierungsfunktionen entfernt:
  - `RegisterVecFontStreamTypes()`
  - `RegisterVecTextStreamTypes()`
  - Aufruf in `MainWindowTextDock.cpp`
  - Aufruf in `ved_core_text_tests`
- `VecStreamFourCC(...)` entfernt. FourCC kommt jetzt zentral ueber
  `VEDMakeFourCC(...)` und die typbezogenen `StreamFourCC()`-Methoden.
- `ved_core_serialization_tests` nutzt fuer moderne Tests kein
  `sc_simple_stream.h` mehr. Die frueheren Cross-Kompatibilitaetstests gegen
  `TDSimpleInStream`/`TDSimpleOutStream` sind nach erfolgter Migration entfernt.

Aktueller Rest:

- `TDSimpleInStream`, `TDSimpleOutStream`, `TDStreamable` und
  `sc_simple_stream.h` tauchen nur noch in `tests/ved_core_sclib_tests.cpp` und
  in der alten `src/ved_core/sclib` selbst auf.
- Damit ist der fachliche Core-Pfad auf `VEDBinaryReader` /
  `VEDBinaryWriter` umgestellt. Das naechste Arbeitspaket kann die isolierte
  alte SCLIB-Testabdeckung entfernen und die letzten SCLIB-Dateien aus dem
  aktiven Build nehmen.

Verifikation:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 21/21 Tests bestanden.

### Umsetzungsnotiz Arbeitspaket 5, 2026-05-18

Erledigt:

- `src/ved_core/sclib` vollstaendig aus dem Workspace entfernt.
- `tests/ved_core_sclib_tests.cpp` entfernt.
- `ved_core_sclib_tests` aus CMake entfernt:
  - kein `add_executable`
  - kein `target_link_libraries`
  - kein `add_test`
- Alle SCLIB-Quellen aus `ved_core` entfernt:
  - `sc_dynamic_factory.*`
  - `sc_memory.*`
  - `sc_simple_stream.*`
  - `sc_stream.*`
  - `sc_streamable.*`
  - bereits vorher entfernte Array-/Utility-Dateien sind nun auch nicht mehr
    als Ordnerreste vorhanden.
- SCLIB-Include-Pfad aus `target_include_directories(ved_core)` entfernt.
- SCLIB-Plattformdefinitionen aus CMake entfernt:
  - `__DANAK_WINDOWS`
  - `__DANAK_LINUX`
- Letzte historische Kommentarzeile mit `sc_trace.h` aus
  `src/ved_core/main/legacy_pending/vec_edit_cad.cpp` entfernt.

Restsuchen:

- `find src -path '*sclib*' -print`
  - keine Treffer
- `grep -R "sc_\|TDStream\|TDSimpleInStream\|TDSimpleOutStream\|TDStreamable\|TDDynamicFactory\|MemoryAlloc\|MemoryReAlloc\|MemoryFree\|StringCopy\|StringLen\|StringSize\|__DANAK_WINDOWS\|__DANAK_LINUX" -n src tests CMakeLists.txt`
  - keine Treffer
- `grep -R "sclib\|sc_" -n CMakeLists.txt src tests`
  - keine Treffer

Verifikation:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 20/20 Tests bestanden.

### Umsetzungsnotiz Arbeitspaket 6, 2026-05-18

Erledigt:

- Testmigration explizit abgeschlossen und abgesichert.
- `ved_core_sclib_tests` bleibt entfernt; die alte SCLIB-Testabdeckung wird
  nicht mehr gebraucht, weil die fachlichen Persistenzpfade modern getestet
  werden.
- `ved_core_serialization_tests` prueft weiterhin:
  - primitive Werte mit fester Breite
  - FourCC
  - Fehlerfaelle
  - `TDVecPolyCurve`-Roundtrip
- `ved_core_text_tests` prueft moderne Roundtrips fuer:
  - `TDVecText`
  - `TDVecFrameText`
- `ved_core_vec_font_tests` prueft jetzt zusaetzlich:
  - `wps_default.vfn` kann weiterhin ueber `LoadVecFontFromMemory(...)`
    geladen werden.
  - geladener `TDVecFont` kann ueber `VEDBinaryWriter` geschrieben werden.
  - geschriebener Font kann ueber `VEDBinaryReader` und `TDVecFont::ReadFrom`
    wieder gelesen werden.
  - Zeichenanzahl, Metriken, Fontname, Glyph `A` und Space-Glyph bleiben im
    Roundtrip erhalten.

Restsuchen:

- `find src -path '*sclib*' -print`
  - keine Treffer
- `grep -R "sclib\|sc_\|TDStream\|TDSimpleInStream\|TDSimpleOutStream\|TDStreamable\|TDDynamicFactory\|MemoryAlloc\|MemoryReAlloc\|MemoryFree\|StringCopy\|StringLen\|StringSize\|__DANAK_WINDOWS\|__DANAK_LINUX" -n CMakeLists.txt src tests`
  - keine Treffer
- `grep -R "ved_core_sclib_tests" -n CMakeLists.txt tests src`
  - keine Treffer

Verifikation:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 20/20 Tests bestanden.

### Umsetzungsnotiz Arbeitspaket 7, 2026-05-18

Finale Abnahme erledigt.

Restsuchen:

- `find src -path '*sclib*' -print`
  - keine Treffer
- `grep -R "sclib\|sc_\|TDStream\|TDSimpleInStream\|TDSimpleOutStream\|TDStreamable\|TDDynamicFactory\|MemoryAlloc\|MemoryReAlloc\|MemoryFree\|StringCopy\|StringLen\|StringSize\|__DANAK_WINDOWS\|__DANAK_LINUX" -n CMakeLists.txt src tests`
  - keine Treffer
- `grep -R "ved_core_sclib_tests" -n CMakeLists.txt tests src`
  - keine Treffer

Verifikation:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 20/20 Tests bestanden.

Damit ist die Story abgeschlossen: `sclib` ist aus aktivem Code, Tests und
Build-Konfiguration entfernt; die fachlichen Persistenzpfade laufen ueber
`VEDBinaryReader` / `VEDBinaryWriter`.

### Abnahmekriterien

- `find src -path '*sclib*'` findet keine produktiven Core-Dateien mehr.
- `grep -R "sc_" src tests CMakeLists.txt` findet keine SCLIB-Includes oder
  SCLIB-Typen mehr.
- Keine Verwendung von:
  - `TDStream`
  - `TDSimpleInStream`
  - `TDSimpleOutStream`
  - `TDStreamable`
  - `TDDynamicFactory`
  - `Memory*`
  - `String*` aus alter SCLIB
- Alle bisherigen Core-Tests laufen.
- Neue Serialisierungstests beweisen:
  - primitive Werte lesen/schreiben mit fester Breite.
  - FourCC-Typen werden korrekt erkannt.
  - `TDVecPolyCurve` Roundtrip.
  - `TDVecText` und `TDVecFrameText` Roundtrip.
  - `TDVecFont` / `wps_default.vfn` kann geladen werden.
- Windows-Build kompiliert ohne SCLIB-Plattformschalter.

## Risiken

### Binaerkompatibilitaet

Das groesste Risiko liegt in der alten Streamsemantik. Borland-Win32 hatte
`long == 4 Byte`. Der aktuelle Port hat bereits Anpassungen, damit `long` in
Streams als 32-bit-Wert geschrieben/gelesen wird. Die neue Serialisierung muss
das explizit beibehalten.

Gegenmassnahme:

- feste Typbreiten in Reader/Writer
- Tests mit vorhandenen `.vfn`-Daten
- Roundtrip-Tests fuer Text/FrameText/PolyCurve

### Ownership-Semantik

`TDObjPtrArray` loescht enthaltene Objekte und kopiert per `Clone()`. Beim Ersatz
durch `std::vector<std::unique_ptr<T>>` muss diese Semantik erhalten bleiben.

Gegenmassnahme:

- Copy-Konstruktoren und Zuweisungsoperatoren gezielt testen.
- `Drop...()`-Methoden als Ownership-Transfer pruefen.

### Zu grosser Schritt in Iteration 3

Die Stream-/Factory-Schicht betrifft mehrere Klassen gleichzeitig. Der Umbau ist
machbar, aber muss mit Tests eng gefuehrt werden.

Gegenmassnahme:

- Reader/Writer zuerst neben alter Stream-Schicht einfuehren.
- Pro Typ migrieren.
- Alte `sclib` erst entfernen, wenn alle betroffenen Typen migriert sind.

## Machbarkeit

Die Entfernung der kompletten `sclib` in drei Iterationen ist machbar.

Die ersten zwei Iterationen sind relativ kontrolliert:

- Iteration 1 entfernt Utility-Altlasten.
- Iteration 2 ersetzt Container und Ownership.

Iteration 3 ist die eigentliche harte Grenze, weil dort Persistenz und alte
Binaerkompatibilitaet haengen. Sie ist trotzdem realistisch, wenn wir sie nicht
als mechanisches Loeschen behandeln, sondern als gezielte Einfuehrung einer
modernen `ved_core/serialization` mit Tests gegen reale Font-/Streamdaten.

Nach Iteration 3 darf es keine SCLIB-Reste mehr geben. Falls waehrend der Arbeit
eine SCLIB-Funktion doch noch fachlich benoetigt wird, wird sie nicht als
SCLIB-Kompatibilitaetswrapper uebernommen, sondern direkt in modernes C++ oder
in eine klar benannte Core-Fachkomponente uebersetzt.
