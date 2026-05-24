# Story: BSpline-Text Performance — Teil 2

Datum: 2026-05-24
Status: offen
Vorgaenger: closed_stories/story_bspline_text_performance_teil-1.md

## Kontext

Teil 1 (Schritte 1-7) hat Core-Caching fuer BSpline, PolyCurve und Text eingefuehrt
sowie adaptive Pixel-Ausduennung im Qt-Rendering. Falls nach diesen Optimierungen
weiterhin messbare Engpaesse bei sehr vielen Objekten bestehen, kann ein Qt-seitiger
Polyline-Cache die Rendering-Seite weiter entlasten.

## Schritt 8: Qt-Polyline-Cache nur falls noetig

Falls nach Core-Cache immer noch Engpaesse bleiben:

- pro View/Zoom einen Screen-Polyline-Cache pruefen.
- `TDGraphicEngineQt::DrawPolygon(...)` koennte redundant gleiche Punktlisten vermeiden.
- Aktuell wird pro Paint ein neues `QPolygonF` aus den gecachten Core-Punkten erzeugt.
- Ein View-Cache koennte das `QPolygonF` wiederverwenden, solange weder Zoom/Pan noch
  die Quelldaten sich geaendert haben.

Dieser Schritt ist groesser und UI-spezifischer. Deshalb erst nach Messung.

### Hinweise

- Object-Level-Culling via `IsRectVisible(pObject->GetFrame())` ist bereits vorhanden
  und verhindert Draw-Aufrufe fuer komplett unsichtbare Objekte.
- Punkt-Level-Clipping innerhalb eines Objekts existiert nicht — Qt uebernimmt das
  Clipping fuer teilweise sichtbare Polylines intern.
- Bei extrem vielen Kontrollpunkten in einem einzelnen Objekt koennte Punkt-Level-Clipping
  relevant werden, ist aber ein separates Thema.

## Akzeptanzkriterien

- Messbare Verbesserung bei wiederholtem Repaint ohne Objektaenderung.
- Kein visueller Unterschied.
- Cache-Invalidierung bei Zoom, Pan, Objektaenderung.
- Bestehende Tests bleiben erfolgreich.
