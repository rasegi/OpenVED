# Story: Multi-View fuer ein Model

Datum: 2026-05-15

## Kontext

VED hat Model und View bereits konzeptionell getrennt. Aktuell wird das Model aber praktisch ueber eine zentrale Canvas betrachtet und bearbeitet.

Zukuenftig sollen mehrere Views gleichzeitig auf dasselbe Model moeglich sein.

Beispiele:

- eine View zeigt eine Gesamtuebersicht oder Preview,
- eine zweite View ist stark hineingezoomt fuer Detailarbeit,
- eine dritte View koennte nur bestimmte Layer oder Hilfen anzeigen,
- eine View ist read-only Preview, eine andere editierbar.

## Ziel

Mehrere Views auf dasselbe `TDVecModel` ermoeglichen.

Jede View soll eigene View-Zustaende haben:

- Zoom
- Pan/Scroll
- sichtbare Hilfen wie Grid, Ruler, Crosshair
- sichtbare Layer
- Preview-/Edit-Modus

Das Model bleibt geteilt.

## Architekturvorschlag

### Core/View State trennen

View-Zustand gehoert nicht ins Model.

Vorschlag:

```cpp
struct TDVecViewSettings {
    bool showGrid = false;
    bool showRulers = false;
    bool showMouseToleranceCross = false;
    bool readOnly = false;
};

struct TDVecViewState {
    double zoom = 1.0;
    long panX = 0;
    long panY = 0;
    TDVecViewSettings settings;
};
```

`TDGraphicEngineScreenState` ist bereits nahe an einem per-View-State. Wichtig ist, dass jede View ihre eigene GraphicEngine/ScreenState-Instanz haben kann.

### View Registry

Qt-seitig:

```cpp
class QVedDocumentView {
public:
    QVedWidget* canvas();
    TDGraphicEngineQt* graphicEngine();
    TDVecViewState viewState() const;
    void applyViewState(const TDVecViewState& state);
};
```

`MainWindow` oder ein `DocumentViewManager` verwaltet mehrere Views.

### Synchronisation

Wenn das Model geaendert wird:

- alle Views refreshen.
- Selektion kann global im Model bleiben oder spaeter view-spezifisch werden.

Offene Entscheidung:

- Ist Selektion Teil des Models oder Teil der View?
- Aktuell liegt Selektion an Objekten im Model.
- Fuer Multi-View kann das zunaechst so bleiben, spaeter aber ueberdacht werden.

## UI-Ideen

- View-Menue:
  - New View
  - New Preview View
  - Tile Views
  - Close View
- Dock-/Tab-System:
  - mehrere Canvas-Tabs
  - optional Splits fuer paralleles Arbeiten
- Preview View:
  - read-only
  - eigene Zoom/Layer-Einstellungen

## Plugin-Bezug

Plugins koennen spaeter eigene View-Typen anbieten:

- Print Preview
- SVG/DXF Export Preview
- Semantic Diagram Preview
- Layer-specific View

## Akzeptanzkriterien

- Mehrere Views koennen dasselbe Model anzeigen.
- Zoom/Pan sind pro View getrennt.
- Grid/Ruler/Crosshair sind pro View getrennt schaltbar.
- Model-Aenderungen werden in allen Views sichtbar.
- Eine Preview-View kann parallel zu einer Detail-Edit-View offen sein.
- Core bleibt unabhaengig von Qt-UI-Layoutentscheidungen.
