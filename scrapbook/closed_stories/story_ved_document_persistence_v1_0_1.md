# Story: VED Dokument-Persistence 1.0.1 / Dateiformat 1.0.2

Datum: 2026-05-18
Status: offen
Zielversion App: 1.0.1
Zielversion Dateiformat: 1.0.2

## Kontext

Nach Entfernung der alten `sclib` basiert der Core auf moderner C++-
Serialisierung mit `VEDBinaryReader` und `VEDBinaryWriter`.

Aktuell existiert noch keine vollstaendige Dokument-/Model-Persistence fuer VED.
Einzelne Core-Typen koennen bereits modern serialisiert werden, aber ein
komplettes `TDVecModel` kann noch nicht als Datei gespeichert und wieder geladen
werden.

Es gibt keine Anforderung an Rueckwaertskompatibilitaet zu alten VED-Dateien.
Es existiert kein altes `.ved`-Format, das erhalten oder rekonstruiert werden
muss. Deshalb wird ein neues, klares und versioniertes VED-Dateiformat
eingefuehrt. Kurzlebige MVP-Dateien aus Dateiformat `1.0.1` werden bewusst
nicht per Legacy-Reader migriert.

## Ziel

VED 1.0.1 soll Dokumente ueber die Qt-Toolbar speichern und oeffnen koennen.

Der MVP umfasst:

- neues versioniertes VED-Dateiformat
- Schreiben eines kompletten `TDVecModel`
- Lesen eines kompletten `TDVecModel`
- Dokument-View-State fuer komfortables Wiedereroeffnen
- Core-Objekte ohne Plugin-Persistence serialisieren
- Open-Dialog ueber Qt
- Save-Dialog ueber Qt
- Font-Fallback beim Laden
- Benutzerwarnung bei fehlenden Fonts oder Lade-/Speicherfehlern

## Nicht-Ziele

- Keine Rueckwaertskompatibilitaet zu alten VED-Dateien.
- Keine Plugin-Persistence.
- Keine plugin-erweiterbaren Metadaten.
- Kein altes `sclib`-/Streamable-/Factory-Modell wieder einfuehren.
- Kein eigenes Qt-Dialogsystem bauen.
- Kein Log-Fenster als Voraussetzung fuer den MVP.

## Formatentscheidung

Es wird ein neues modernes VED-Format geschrieben.

Vorgeschlagener Container:

```text
Magic:   VEDM
Version: 1.0.2
Chunk*

Chunk:
  FourCC
  Payload length
  Payload

Aktuelle Chunks:
  META  Model metadata
  VIEW  Document view state, optional
  OBJS  Object count + object records
```

Jedes Objekt wird mit einer festen Core-Typkennung geschrieben:

```text
Object FourCC / Type
Object payload
```

Da Plugin-Persistence explizit nicht Teil dieses MVP ist, reicht fuer die
erste Version eine feste Core-Dispatch-Logik fuer die bekannten
Core-Objekttypen. Unbekannte Dokument-Chunks koennen uebersprungen werden;
unbekannte Objekt-Typen bleiben ein kontrollierter Ladefehler.

## Dokument-View-State

Neben dem eigentlichen Model soll das Dokument auch UI-nahe View-Informationen
speichern, damit ein Dokument beim Oeffnen wieder in einem sinnvollen
Arbeitszustand erscheint.

Dieser View-State ist kein fachlicher Objektinhalt. Deshalb wird er als eigener
Qt-freier Abschnitt im Dokument behandelt.

Zu speichern:

- Zoom-Faktor
- Mittelpunkt der View in Modellkoordinaten
- Grid sichtbar
- Grid-Snap/Grid-Lock aktiv
- Grid-Abstand X/Y
- Lineale sichtbar

Grundsatz:

- Keine Qt-Typen im Core-Format speichern.
- Keine Fenster-/Monitor-spezifischen Pixelkoordinaten speichern.
- Kein Multi-View-State fuer den MVP.
- Wenn der optionale `VIEW`-Chunk fehlt oder nicht lesbar ist, soll das
  Dokument trotzdem mit sinnvollen Defaults ladbar bleiben.

Defaults bei fehlendem View-State:

- Zoom auf App-/View-Standard
- Mittelpunkt auf Dokument- oder Model-Area-Mitte
- Grid/Lock/Lineale nach App-Default

## Core-Objekttypen

Folgende Typen muessen vom Dokumentformat unterstuetzt werden:

- `TDVecLine`
- `TDVecEllipse`
- `TDVecPolygon`
- `TDVecPolygonArea`
- `TDVecBSPLine`
- `TDVecBezierCurve`
- `TDVecText`
- `TDVecFrameText`
- `TDVecGroup`
- `TDVecPolyCurve`
- `TDVecPolyCurveArea`

Bereits vorhandene moderne Serialisierung soll weiterverwendet werden.
Fehlende Typen bekommen eigene `WriteTo`-/`ReadFrom`-Logik oder gleichwertige
typbezogene Serializer im Core.

## Font-Fallback

Textobjekte speichern keine rohen Font-Pointer, sondern einen stabilen Fontnamen
oder eine stabile Fontreferenz.

Beim Laden gilt:

1. Gewuenschten Font per gespeicherten Namen aufloesen.
2. Wenn der Font fehlt, auf `Arial Unicode MS` ausweichen.
3. Wenn `Arial Unicode MS` ebenfalls fehlt, auf `wps_default` ausweichen.
4. Fuer den MVP wird der Benutzer ueber eine Qt-Infobox/Warnbox informiert.
5. Spaeter wird diese Meldung in ein Log-Fenster verschoben.

Die Font-Warnung darf das Dokument nicht unnoetig blockieren, solange ein
Fallback-Font verfuegbar ist.

## Qt-Integration

Qt bringt die benoetigten Standarddialoge mit:

- `QFileDialog::getOpenFileName(...)`
- `QFileDialog::getSaveFileName(...)`
- `QMessageBox::warning(...)`
- `QMessageBox::information(...)`

Fuer den MVP werden Open und Save ueber die bestehende Toolbar eingebunden.

## Arbeitspaket 1: Dokumentformat und Model-I/O im Core

Status: umgesetzt am 2026-05-18

### Ziel

Ein `TDVecModel` kann im Speicher in das neue VED-Format geschrieben und aus
diesem Format wieder gelesen werden.

### Arbeiten

1. Dateisignatur und Versionsheader definieren.
2. Model-Metadaten schreiben:
   - sichtbarer/gueltiger Modellbereich
   - Default-Farbe
   - weitere aktuell notwendige Modeldaten
3. Objektanzahl schreiben.
4. Objektliste schreiben und lesen.
5. Fehlerfaelle sauber melden:
   - falsches Magic
   - nicht unterstuetzte Version
   - abgeschnittener Stream
   - unbekannter Core-Objekttyp

### Abnahmekriterien

- Ein leeres Model kann gespeichert und geladen werden.
- Model-Metadaten bleiben im Roundtrip erhalten.
- Fehlerhafte Daten erzeugen kontrollierte Fehler statt Absturz.

### Umsetzungsnotiz 2026-05-18

Erledigt:

- `ved_model_io.*` im Core angelegt.
- Format-Header `VEDM` mit Version `1.0.1` eingefuehrt; spaeter fuer
  chunkbasierten View-State auf Dateiformat `1.0.2` angehoben.
- Speicher-API fuer `SaveVecModelToBytes(...)` und
  `LoadVecModelFromBytes(...)` eingefuehrt.
- Model-Metadaten fuer leere Models werden geschrieben und gelesen.
- Fehlerstatus fuer ungueltige Signatur, nicht unterstuetzte Version,
  unbekannte Objekttypen und nicht unterstuetzte Objekttypen ergaenzt.
- `ved_core_model_io_tests` ergaenzt.

Noch bewusst offen fuer Arbeitspaket 2:

- nicht-leere Objektlisten werden noch nicht persistiert.
- Core-Objekt-Dispatcher und konkrete Objekt-Serializer folgen im naechsten
  Arbeitspaket.

## Arbeitspaket 2: Core-Objekt-Dispatcher

Status: umgesetzt am 2026-05-18

### Ziel

Objekte werden ohne Plugin-System anhand ihrer Core-Typkennung gelesen und
geschrieben.

### Arbeiten

1. Schreibpfad:
   - Objekttyp/FourCC schreiben
   - objektbezogenen Payload schreiben
2. Lesepfad:
   - Typkennung lesen
   - passenden Core-Typ erzeugen
   - Payload lesen
3. Parent-/Model-Beziehungen nach dem Laden korrekt setzen.
4. Gruppen rekursiv behandeln.

### Abnahmekriterien

- Alle bekannten Core-Typen koennen dispatcht werden.
- Unbekannte Typen erzeugen einen klaren Ladefehler.
- Gruppen koennen verschachtelte Core-Objekte enthalten.

### Umsetzungsnotiz 2026-05-18

Erledigt:

- `ved_object_io.*` als Qt-freier Core-Dispatcher eingefuehrt.
- Objekt-Records im Dokumentformat eingefuehrt:
  - Objekt-`FourCC`
  - Payload-Laenge
  - Payload
- `ved_model_io` schreibt und liest Objektlisten ueber diesen Dispatcher.
- Bereits vorhandene moderne Serializer sind in den Dokumentpfad integriert:
  - `TDVecPolyCurve`
  - `TDVecText`
  - `TDVecFrameText`
- Unbekannte Objekt-`FourCC`s erzeugen `VEDBinaryError::UnknownObjectType`.
- Noch nicht serialisierbare Core-Objekte erzeugen beim Speichern
  `VEDBinaryError::UnsupportedObjectType`.
- Parent-/Model-Beziehung wird beim Laden ueber `TDVecModel::AppendObject`
  gesetzt.
- Roundtrip-Test fuer ein Model mit `TDVecPolyCurve` ergaenzt.
- Fehlerfall fuer unbekannte Objektkennung ergaenzt.

Noch offen fuer Arbeitspaket 3:

- `TDVecLine`
- `TDVecEllipse`
- `TDVecPolygon`
- `TDVecPolygonArea`
- `TDVecBSPLine`
- `TDVecBezierCurve`
- `TDVecGroup`
- `TDVecPolyCurveArea`
- Gruppen-Rekursion, sobald `TDVecGroup` selbst serialisierbar ist.

## Arbeitspaket 3: Fehlende Objekt-Serializer

Status: umgesetzt am 2026-05-18

### Ziel

Alle aktuellen Core-Objekttypen koennen verlustfrei fuer den aktuellen
Funktionsumfang gespeichert und geladen werden.

### Arbeiten

1. `TDVecLine` serialisieren.
2. `TDVecEllipse` serialisieren.
3. `TDVecPolygon` serialisieren.
4. `TDVecPolygonArea` serialisieren.
5. `TDVecBSPLine` serialisieren.
6. `TDVecBezierCurve` serialisieren.
7. `TDVecGroup` serialisieren.
8. `TDVecPolyCurveArea` serialisieren.
9. Bestehende Serializer fuer Text, FrameText und PolyCurve in den
   Dokumentpfad integrieren.

### Grundsatz

Abgeleitete Daten und Caches werden nicht als primaere Wahrheit persistiert.
Gespeichert werden Eingabedaten und stabile Einstellungen. Nach dem Laden werden
Geometrie, Frames, Bounds und Caches neu aufgebaut.

### Abnahmekriterien

- Jeder Core-Objekttyp hat einen eigenen Roundtrip-Test.
- Der geladene Zustand entspricht dem gespeicherten fachlichen Zustand.
- Kurven und Gruppen bleiben nach dem Laden bedienbar.

### Umsetzungsnotiz 2026-05-18

Erledigt:

- Serializer fuer einfache Geometrietypen eingefuehrt:
  - `TDVecLine`
  - `TDVecEllipse`
  - `TDVecPolygon`
  - `TDVecPolygonArea`
- Serializer fuer Kurven-/Area-Typen eingefuehrt:
  - `TDVecBezierCurve`
  - `TDVecBSPLine`
  - `TDVecPolyCurveArea`
- `TDVecGroup` rekursiv serialisierbar gemacht.
- Der Core-Dispatcher kennt jetzt alle aktuellen Core-Objekttypen:
  - `TDVecLine`
  - `TDVecEllipse`
  - `TDVecPolygon`
  - `TDVecPolygonArea`
  - `TDVecBSPLine`
  - `TDVecBezierCurve`
  - `TDVecText`
  - `TDVecFrameText`
  - `TDVecGroup`
  - `TDVecPolyCurve`
  - `TDVecPolyCurveArea`
- Roundtrip-Tests im `ved_core_model_io_tests` erweitert:
  - einfache Geometrieobjekte
  - Kurvenobjekte
  - PolyCurveArea
  - Gruppe mit verschachtelten Objekten
- Caches und abgeleitete Geometrie werden nicht als primaere Wahrheit
  persistiert. Gespeichert werden Eingabedaten und stabile Einstellungen;
  Kurven-/Bounds-Daten werden nach dem Lesen neu aufgebaut.

Verifiziert:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 21/21 Tests erfolgreich.

## Arbeitspaket 4: Font-Aufloesung und MVP-Warnungen

Status: Core-Anteil umgesetzt am 2026-05-18

### Ziel

Dokumente mit Text bleiben auch dann ladbar, wenn der urspruengliche Font auf
dem Zielsystem fehlt.

### Arbeiten

1. Fontnamen aus Textobjekten lesen.
2. Font im aktuellen Fontbestand suchen.
3. Fallback auf `Arial Unicode MS`.
4. Fallback auf `wps_default`.
5. Warninformation sammeln.
6. Im MVP nach dem Laden per `QMessageBox` anzeigen.

### Abnahmekriterien

- Fehlender Font verhindert das Laden nicht, wenn ein Fallback vorhanden ist.
- Benutzer sieht eine Warnung.
- Textobjekte besitzen nach dem Laden einen gueltigen Font.

### Umsetzungsnotiz 2026-05-18

Erledigt:

- `TDFontManager::GetVecFontExact(...)` eingefuehrt, damit der Loader
  unterscheiden kann zwischen:
  - gewuenschter Font wurde gefunden
  - gewuenschter Font fehlt und wurde ersetzt
- `ved_font_resolution.*` als Qt-freie Core-Schicht eingefuehrt.
- Fallback-Reihenfolge implementiert:
  1. gespeicherter Fontname
  2. `TT:Arial Unicode MS`
  3. `VC:WPS Default`
- `LoadVecModelFromBytes(...)` kann optional einen `TDFontManager` bekommen und
  danach Font-Aufloesung fuer das geladene Model ausfuehren.
- Font-Warnungen werden im `VEDModelReadResult` gesammelt.
- Textobjekte bekommen nach erfolgreichem Fallback einen gueltigen
  `TDVecFont`-Pointer und den verwendeten Fontnamen.
- Verschachtelte Textobjekte in `TDVecGroup` werden ebenfalls aufgeloest.
- Tests im `ved_core_model_io_tests` ergaenzt:
  - fehlender Font faellt auf `TT:Arial Unicode MS` zurueck
  - fehlender Font in Gruppe faellt auf `VC:WPS Default` zurueck

Noch offen fuer Arbeitspaket 6:

- Die gesammelten Font-Warnungen muessen im Qt-Open-Pfad per
  `QMessageBox::warning(...)` angezeigt werden.

Verifiziert:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 21/21 Tests erfolgreich.

## Arbeitspaket 5: Dokument-View-State

Status: umgesetzt am 2026-05-18

### Ziel

Zoom, Grid, Grid-Lock, Lineale und View-Mittelpunkt werden mit dem Dokument
gespeichert und beim Oeffnen wiederhergestellt.

### Arbeiten

1. Qt-freie Core-Struktur fuer den Dokument-View-State definieren.
2. View-State im VED-Format schreiben.
3. View-State beim Laden lesen.
4. Tolerantes Laden definieren:
   - nicht gesetzter View-State fuehrt zu Defaults
   - kaputter View-State fuehrt zu Defaults, nicht zu Dokumentverlust
5. App-/View-Layer an die Core-Struktur anbinden.

### Umsetzung

- `VEDDocumentViewState` als Qt-freie Core-Struktur eingefuehrt.
- Dateiformat auf `1.0.2` angehoben.
- Dokumentkoerper auf Chunk-Struktur umgestellt:
  - `META` fuer Model-Metadaten
  - `VIEW` fuer optionalen Dokument-View-State
  - `OBJS` fuer Objektanzahl und Objektdatensaetze
- Unbekannte Dokument-Chunks werden beim Laden uebersprungen.
- Fehlender oder defekter `VIEW`-Chunk faellt auf Default-View-State zurueck.
- Alte `1.0.1`-MVP-Dateien werden ohne Legacy-Reader als
  `UnsupportedVersion` abgelehnt.
- `SaveVecModelToBytes(...)` hat eine Variante mit View-State bekommen.
- Das bestehende `SaveVecModelToBytes(model)` schreibt einen nicht gesetzten
  View-State und bleibt fuer Tests/Core-Aufrufer nutzbar.
- `LoadVecModelFromBytes(...)` liefert den gelesenen View-State im
  `VEDModelReadResult`.
- `QVedWidget` kann View-State aus Zoom, View-Mittelpunkt, Grid, Grid-Lock und
  Linealen erzeugen und wieder anwenden.
- `MainWindow::saveDocument()` speichert den aktuellen Canvas-View-State.
- `MainWindow::openDocument()` uebergibt den geladenen View-State an
  `installModel(...)`; dort wird nicht mehr pauschal `resetView()` aufgerufen,
  wenn ein View-State vorhanden ist.
- Toolbar-Buttons fuer Grid, Grid-Lock und Lineale werden nach dem Laden mit
  dem Canvas synchronisiert.

### Abnahmekriterien

- Zoom-Faktor bleibt nach Save/Open erhalten.
- View-Mittelpunkt bleibt nach Save/Open erhalten.
- Grid sichtbar bleibt nach Save/Open erhalten.
- Grid-Snap/Grid-Lock bleibt nach Save/Open erhalten.
- Lineal-Sichtbarkeit bleibt nach Save/Open erhalten.
- Datei bleibt ladbar, wenn der View-State als nicht gesetzt markiert ist.

Verifiziert:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 21/21 Tests erfolgreich.

## Arbeitspaket 6: Toolbar Open/Save

Status: vorgezogen und umgesetzt am 2026-05-18

### Ziel

Der Benutzer kann VED-Dokumente ueber die Anwendung oeffnen und speichern.

### Arbeiten

1. Toolbar-Aktion `Open` ergaenzen oder anbinden.
2. Toolbar-Aktion `Save` ergaenzen oder anbinden.
3. `QFileDialog::getOpenFileName(...)` fuer Open verwenden.
4. `QFileDialog::getSaveFileName(...)` fuer Save verwenden.
5. Dateiendung `.ved` anbieten.
6. Ladefehler per `QMessageBox::warning(...)` anzeigen.
7. Speicherfehler per `QMessageBox::warning(...)` anzeigen.
8. Nach erfolgreichem Laden View/Model aktualisieren.

### Abnahmekriterien

- Benutzer kann eine `.ved`-Datei speichern.
- Benutzer kann dieselbe Datei wieder oeffnen.
- Fehler werden sichtbar gemeldet.
- Die Anwendung bleibt nach fehlgeschlagenem Laden in einem konsistenten
  Zustand.

### Umsetzungsnotiz 2026-05-18

Dieses Arbeitspaket wurde vor Arbeitspaket 5 umgesetzt, damit der aktuelle
Dokument-Persistence-Stand manuell ueber die UI testbar ist.

Erledigt:

- Toolbar-Buttons `Open` und `Save` aktiviert.
- File-Menue-Eintraege `Open...` und `Save...` mit Standard-Shortcuts
  ergaenzt.
- `QFileDialog::getOpenFileName(...)` fuer Open angebunden.
- `QFileDialog::getSaveFileName(...)` fuer Save angebunden.
- `.ved` als Dateiendung/Filter angeboten.
- `Save` schreibt das aktuelle `TDVecModel` ueber `SaveVecModelToBytes(...)`.
- `Open` liest `.ved` ueber `LoadVecModelFromBytes(...)`.
- Nach erfolgreichem Open wird das geladene Model in Editor,
  OperationManager und Canvas installiert.
- Nach Open wird auf Select-Operation zurueckgeschaltet und die View
  aktualisiert.
- Font-Fallback-Warnungen aus dem Core werden per `QMessageBox::warning(...)`
  angezeigt.
- Lade- und Speicherfehler werden per `QMessageBox::warning(...)` angezeigt.

Verifiziert:

- `cmake --build cmake-build-debug`
- `ctest --test-dir cmake-build-debug --output-on-failure`
- Ergebnis: 21/21 Tests erfolgreich.

Noch offen:

- Manueller UI-Test in der laufenden App.
- Dirty-State/Save-without-dialog ist noch nicht Teil des MVP.

## Arbeitspaket 7: Tests

### Ziel

Die neue Persistence wird gegen Regressionen abgesichert.

### Tests

- leeres Model speichern/laden
- Model mit mehreren Geometrieobjekten
- Model mit Text und FrameText
- Model mit Gruppe und verschachtelten Objekten
- Model mit Kurvenobjekten
- View-State speichern/laden
- nicht gesetzter View-State mit Defaults
- fehlender Font mit Fallback
- falsches Magic
- nicht unterstuetzte Version
- abgeschnittener Stream
- unbekannter Core-Objekttyp

### Abnahmekriterien

- Core-Tests laufen stabil.
- Dokument-Roundtrip ist fuer den MVP abgedeckt.
- Fehlerfaelle sind getestet.

## Aufwandsschaetzung

Ohne Rueckwaertskompatibilitaet und ohne Plugin-Persistence ist die Story
realistisch in etwa 5 bis 8 fokussierten Entwicklungstagen umsetzbar.

Der Aufwand verteilt sich grob:

- Dokumentformat und Model-I/O: 1 bis 2 Tage
- fehlende Objekt-Serializer: 2 bis 3 Tage
- Font-Fallback: 0.5 bis 1 Tag
- Dokument-View-State: 0.5 bis 1 Tag
- Qt-Toolbar/Open/Save: 0.5 bis 1 Tag
- Tests und Stabilisierung: 1 bis 2 Tage

## Definition of Done

- VED 1.0.1 kann ein neues Dokumentformat speichern.
- VED 1.0.1 kann dasselbe Dokumentformat laden.
- Das Dokumentformat ist versioniert und nutzt Chunks fuer optionale
  Dokumentabschnitte.
- Alle aktuellen Core-Objekttypen werden im Dokumentformat unterstuetzt.
- Dokument-View-State wird gespeichert und beim Oeffnen wiederhergestellt.
- Open und Save sind ueber die Toolbar erreichbar.
- Qt-Standarddialoge werden verwendet.
- Fehlende Fonts werden ueber die definierte Fallback-Kette behandelt.
- Benutzer bekommt fuer den MVP sichtbare Warnungen per Dialog.
- Core- und UI-Tests fuer den MVP laufen.
- Keine neue `sclib`-Abhaengigkeit.
- Keine Plugin-Persistence als versteckte Voraussetzung.
