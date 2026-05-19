# Story: Drag Document und Reset View

Datum: 2026-05-16
Status: geschlossen

## Kontext

Beim Arbeiten mit dem Qt-Port war die View-Navigation zu stark von den Scrollbars abhaengig.
Gerade nach Zoom- und Pan-Aktionen konnte die Ansicht in einen Zustand kommen, der schwer
zu korrigieren war.

Gewuenscht waren zwei praktische View-Werkzeuge:

- eine Moeglichkeit, Zoom und Scroll-/Pan-Zustand wieder auf den App-Startzustand
  zurueckzusetzen,
- ein Tablet-aehnlicher Drag-Modus, mit dem das Dokument per Maus/Finger-Symbol in alle
  Richtungen verschoben werden kann.

## Ziel

Die View soll ohne manuelles Scrollbar-Nachregeln wieder bedienbar werden:

- `Reset View` stellt Zoom, ViewRange, WorldSpace/Margin und Scrollbars auf den Startzustand
  zurueck.
- `Drag Document` erlaubt freies Verschieben der Dokumentansicht mit linker Maustaste.
- Dragging soll auch an den aktuellen WorldSpace-Raendern nicht hart klemmen.
- Zoom-, Pan- und Zeichen-/Aenderungswerkzeuge sollen sich gegenseitig sauber deaktivieren.

## Umsetzung

### Reset View

`QVedWidget::resetView()` initialisiert die View erneut:

- laufendes Panning wird beendet,
- Auswahlrechteck/Snap-Marker werden zurueckgesetzt,
- Device-Metriken werden aktualisiert,
- WorkSpace, WorldSpace und ViewRange werden wieder wie beim Start gesetzt,
- Scrollbars werden ueber die normale View-Initialisierung aktualisiert.

Die Funktion ist weiterhin im `View`-Menue verfuegbar und wurde zusaetzlich in die
View-Toolbar aufgenommen.

### Drag Document

`QVedWidget` hat einen neuen Pan-Tool-Zustand erhalten:

- `setPanToolEnabled(bool)`
- `panToolEnabled()`

Wenn der Pan-Modus aktiv ist, startet die linke Maustaste ein direktes Verschieben der
Dokumentansicht. Das Werkzeug nutzt den bestehenden Dock-/Hand-Cursor.

In `MainWindow` wurde eine neue View-Toolbar-Aktion `Pan` mit Hand/Finger-Icon ergaenzt.
Beim Aktivieren von Pan wird Zoom deaktiviert und der aktuelle Zeichen-/Aenderungsmodus
auf `None` gesetzt. Beim Aktivieren eines Zeichen- oder Aenderungswerkzeugs werden Zoom
und Pan gemeinsam deaktiviert.

### Pan ohne Randklemme

Fuer freies Verschieben wurde der Graphic-Engine-State erweitert:

- `TDGraphicEngineScreenState::SetPanAllowingWorldExpansion(long nPanX, long nPanY)`
- Qt-Wrapper: `TDGraphicEngineQt::SetPanAllowingWorldExpansion(...)`

Der neue Pan-Aufruf berechnet den gewuenschten sichtbaren Ausschnitt und erweitert den
WorldSpace bei Bedarf, bevor Pan angewendet wird. Dadurch kann die View in alle Richtungen
verschoben werden, ohne an der bisherigen Scrollbar-Grenze sofort zu klemmen.

## Geaenderte Dateien

- `src/app/QVedWidget.h`
- `src/app/QVedWidget.cpp`
- `src/app/MainWindow.h`
- `src/app/MainWindow.cpp`
- `src/ved_core/gengine/vec_graphic_engine_screen_state.h`
- `src/ved_core/gengine/vec_graphic_engine_screen_state.cpp`
- `src/ved_qt/gengine/vec_graphic_engine_qt.h`
- `src/ved_qt/gengine/vec_graphic_engine_qt.cpp`
- `tests/ved_core_zoom_pan_tests.cpp`

## Akzeptanzkriterien

- Reset View setzt die Ansicht auf den Startzustand zurueck.
- In der View-Toolbar gibt es eine direkte Reset-View-Aktion.
- In der View-Toolbar gibt es einen Pan-/Drag-Document-Modus.
- Im Pan-Modus kann das Dokument mit linker Maustaste verschoben werden.
- Das Verschieben funktioniert auch ueber bisherige WorldSpace-Raender hinaus.
- Scrollbars werden nach Dragging passend aktualisiert.
- Zoom- und Pan-Modus sind gegenseitig exklusiv.
- Zeichen-/Aenderungswerkzeuge deaktivieren aktive View-Werkzeuge.

## Verifikation

Ausgefuehrt:

```text
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
```

Ergebnis:

- Build erfolgreich.
- Alle Tests erfolgreich: 16/16.

## Hinweise

Diese Story behandelt nur Reset View und Drag Document. Die grundsaetzliche Zoom-Mechanik
bleibt ein eigenes sensibles Thema, weil dort exakt erhalten bleiben muss, welcher
Dokumentpunkt unter der Maus liegt.
