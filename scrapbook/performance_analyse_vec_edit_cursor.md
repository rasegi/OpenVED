# Performance-Analyse: vec_edit Cursor bei Mouse-Move

Datum: 2026-05-16
Status: Analyse

## Kontext

Beim Arbeiten im Qt-Port wirkt die Anwendung zeitweise deutlich traeger. Ein konkreter
Verdacht liegt auf dem Cursor-Pfad: `TDVecEditCad::UseCursor(...)` wird sehr haeufig
aufgerufen, teilweise bei jeder Mausbewegung.

Der betreffende Code liegt in:

- `src/ved_core/main/vec_edit_cad.cpp`
- `src/app/QVedWidget.cpp`
- `src/app/MainWindow.cpp`
- `src/ved_core/operations/*`

## Beobachtung

Der Cursor wird nicht nur bei einem tatsaechlichen Cursorwechsel gesetzt. Mehrere
Operationen rufen waehrend `OPMouseMove(...)` wiederholt `mpVecEditCad->UseCursor(...)`
auf, auch wenn sich der Cursor-Typ nicht geaendert hat.

In der Qt-Schicht ist das teurer als in der alten Umgebung, weil der Aufruf bis zur View
weitergereicht wird und dort je nach Cursor ein `QPixmap` aus den Ressourcen geladen und
ein neuer `QCursor` gesetzt wird.

## Aufrufpfad

Der normale Operation-Pfad bei Mouse-Move ist:

```text
QVedWidget::mouseMoveEvent(...)
  -> operationMouseMoveCallback_(...)
    -> MainWindow callback
      -> editor_->MouseMove(...)
        -> TDVecEditCad::MouseMove(...)
          -> TDViewOperationManager::MouseMove(...)
            -> aktive Operation: OPMouseMove(...)
              -> mpVecEditCad->UseCursor(...)
                -> TDVecViewInterfaceBase::Send_UseCursor_ToView(...)
                  -> QVedWidget::useCursor(...)
```

Relevante Stellen:

- `QVedWidget::mouseMoveEvent(...)`
- `MainWindow::initializeEditor(...)`, Callback `setOperationMouseMoveCallback(...)`
- `TDVecEditCad::MouseMove(...)`
- `TDViewOperationManager::MouseMove(...)`
- `TDVecEditCad::UseCursor(...)`
- `QVedWidget::useCursor(...)`

## Zweiter Cursor-Pfad

Zusaetzlich ruft der Qt-Mouse-Move-Callback in `MainWindow` nach jedem
`editor_->MouseMove(...)` auch `updateMouseToleranceCrossCursor()` auf.

Dieser Pfad kann bei Select-Operationen ebenfalls `editor_->UseCursor(...)` ausloesen:

```text
MainWindow mouse move callback
  -> updateMouseToleranceCrossCursor()
    -> editor_->GetUsedCursor()
    -> editor_->UseCursor(VECVIEW_SIMPLE_CROSS oder VECVIEW_CURSOR_DEFAULT)
```

Damit kann ein einzelnes Mouse-Move-Event zwei Cursor-Entscheidungen verursachen:

- eine aus der aktiven Core-Operation,
- eine aus der Qt-UI-Policy fuer Mouse-Tolerance-Cross.

## Beispiele aus Operationen

Viele Operationen setzen den Cursor in `OPMouseMove(...)` bewusst dynamisch:

- `voc_line.cpp`: wechselt zwischen `VECVIEW_CREATE_LINE_1` und
  `VECVIEW_CREATE_LINE_2`.
- `voc_rectangle_notrotated.cpp`: setzt Create-Rectangle-Cursor je nach
  Operationszustand.
- `voc_roundrectangle_notrotated.cpp`: gleicher Mechanismus fuer Round-Rectangle.
- `voc_ellipse_orthogonal.cpp` und `voc_ellipse_midpoint.cpp`: setzen
  Ellipse-Cursor je nach gueltigem Zustand.
- `vom_selectmove_object_scale_frame.cpp`: setzt u.a. `VECVIEW_DOCK`,
  `VECVIEW_NO`, `VECVIEW_PLUS` oder Default je nach Trefferbereich.

Das Muster ist fachlich nachvollziehbar: Die aktive Operation besitzt die Cursor-Logik.
Problematisch ist nur, dass identische Cursorwerte ohne Guard bis in die Qt-Schicht
weitergereicht werden.

## Guard in TDVecEditCad::UseCursor

Der Guard:

```cpp
if (meUsedCursor == eShape) {
    return;
}
```

verhindert, dass identische Cursorwerte wiederholt an die View geschickt werden.

Das reduziert:

- wiederholtes Laden derselben Cursor-Pixmap,
- wiederholtes Erzeugen von `QCursor`,
- wiederholtes `setCursor(...)`,
- unnoetige Arbeit bei jedem Mouse-Move.

## Einschraenkung

Der Guard ist nur dann vollstaendig korrekt, wenn derselbe `TDVecViewCursor` immer
denselben konkreten Qt-Cursor bedeutet.

Im aktuellen Qt-Port gibt es eine wichtige Ausnahme:

- `VECVIEW_DOCK` kann normaler Dock-/Hand-Cursor sein.
- Bei aktivem Drag-Document/Pan-Tool soll derselbe Enum aber den Drag-Document-Cursor
  anzeigen.

Wenn nur im Core nach `eShape` gecacht wird, kann ein UI-Moduswechsel uebersehen werden,
obwohl der konkrete Cursor anders aussehen muesste.

## Bessere robuste Loesung

Langfristig ist ein Cache in `QVedWidget::useCursor(...)` robuster als ein reiner
Enum-Guard im Core.

Der Cache sollte den konkreten Cursor-Schluessel vergleichen, nicht nur den Core-Enum:

- Cursor-Enum,
- Resource-Pfad,
- Hotspot,
- relevanter UI-Modus, z.B. `panToolEnabled_`.

Damit bleibt die alte Core-Operation-Logik unveraendert, aber Qt baut den Cursor nur neu,
wenn sich der tatsaechliche Cursor geaendert hat.

## Bewertung

Die haeufigen `UseCursor(...)`-Aufrufe kommen hauptsaechlich aus der alten
operationsgetriebenen Cursor-Logik. Das ist nicht automatisch ein Bug. Im Qt-Port wird
dieses Muster aber teuer, wenn jeder identische Aufruf bis zu `QPixmap`/`QCursor`/`setCursor`
durchgereicht wird.

Kurzfristig kann der Guard in `TDVecEditCad::UseCursor(...)` die Performance verbessern.
Robuster ist ein zusaetzlicher oder alternativer Cache in `QVedWidget::useCursor(...)`,
weil dort bekannt ist, welcher konkrete Qt-Cursor wirklich gesetzt werden soll.

## Offene Punkte

- Messen, ob die wahrnehmbare Traegheit nach Reaktivierung des Guards verschwindet.
- Pruefen, ob `VECVIEW_DOCK` bei Wechsel von Drag-Document/Pan-Tool sauber aktualisiert
  wird.
- Falls nicht, Cursor-Caching in `QVedWidget::useCursor(...)` auf konkreten Cursor-Key
  umstellen.
- Separat pruefen, ob weitere Mouse-Move-Pfade Performance kosten:
  - Drag-Panning mit `SetPanAllowingWorldExpansion(...)`,
  - Scrollbar-Updates,
  - Rotate-Preview und Objektkopien waehrend Mouse-Move.
