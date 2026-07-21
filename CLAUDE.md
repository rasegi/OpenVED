# OpenVED — Claude Code Projekt-Anweisungen

## Sprache

Kommunikation auf Deutsch. Code, Variablennamen und technische Begriffe bleiben Englisch.

## Architektur

- **VED Core** (`src/ved_core/`) — Qt-unabhaengig. Keine Qt-Header (`QString`, `QPainter`, `QColor` etc.). Nur `std::string`, `double`, eigene Structs.
- **VED Qt** (`src/ved_qt/`) — Qt-spezifischer Code. Darf Core-Typen nutzen, aber nie umgekehrt.
- **App** (`src/app/`) — Qt-Anwendung (MainWindow, QVedWidget, Dialoge).
- **Tests** (`tests/`) — Jedes Test-Target ist eine eigene `main()`-Datei mit `assert()`-basierten Tests.

## Build

```bash
cmake --build cmake-build-debug
ctest --output-on-failure
```

## Workflow

### Story-Branches

- Arbeit immer auf Story-Branches, benannt nach der Story (z.B. `story_bspline_text_performance`).
- Branch von `main` erstellen bevor Aenderungen gemacht werden.
- **Vor dem Merge in main:** Story, Plan und Concept-Dateien von `scrapbook/open_stories/` nach `scrapbook/closed_stories/` verschieben.
- Dann erst in `main` mergen (`--no-ff`).

### Commits

- Commits nur nach expliziter Freigabe durch den User erstellen.
- User will Aenderungen erst selbst testen und abnehmen, bevor committet wird.

### Stories und Plaene

- Stories liegen in `scrapbook/open_stories/` (offen) bzw. `scrapbook/closed_stories/` (erledigt).
- Ein Plan beschreibt Steps mit Was/Tests/Log-Sektionen.
- Log-Eintraege werden nach Umsetzung jedes Steps ausgefuellt.
- Checkboxen in Tests werden abgehakt wenn bestanden.

### Abschluss von Stories

Eine Story wird erst abgeschlossen, wenn sie vom User **abgenommen** ist (funktional getestet / visuell bestaetigt). Ablauf:

1. **Status und Abnahme vermerken:** Jedes Arbeitspaket/Step erhaelt bei Umsetzung den Vermerk "umgesetzt am YYYY-MM-DD". Bei Abschluss wird im Story-Kopf `Status: erledigt (YYYY-MM-DD)` gesetzt — mit kurzem Abnahme-Vermerk (was visuell abgenommen, was per Tests verifiziert wurde). Durch spaetere Steps ueberholte Checkboxen werden als gegenstandslos markiert.
2. **Verschieben:** Die Story-Datei (inkl. zugehoeriger Plan-/Concept-Dateien) wird per `git mv` von `scrapbook/open_stories/` nach `scrapbook/closed_stories/` verschoben (erhaelt die Git-History).
3. **Reihenfolge:** Zuerst verschieben, dann **nach expliziter Freigabe durch den User** in `main` mergen (`--no-ff`).

## Konventionen

- Core muss Qt-unabhaengig bleiben — das ist ein harter Architektur-Grundsatz.
- Real-Einheiten: 1 mm = 1000 Real-Einheiten (default `realUnitsPerMillimeter = 1000.0`).
- Koordinatensystem: (0,0) oben links, X nach rechts, Y nach unten.
- Der Zeichenbereich (Workspace) ist unbegrenzt. Das Papier ist eine visuelle Schablone, kein Limit.
