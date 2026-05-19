# Story: Layer und Objekt-Reihenfolge

Datum: 2026-05-15

## Kontext

Aktuell liegen Objekte im Model in einer Objektliste. Diese Reihenfolge beeinflusst bereits implizit Zeichen- und Trefferverhalten.

Zukuenftig soll VED echte Layer unterstuetzen:

- mehrere Layer pro Dokument,
- Objekte gehoeren zu genau einem Layer,
- Layer koennen sichtbar/unsichtbar sein,
- Layer koennen gesperrt sein,
- Objekte koennen zwischen Layern verschoben werden,
- Objekt-Reihenfolge innerhalb eines Layers ist steuerbar.

## Ziel

Layer als Core-Konzept im Model einfuehren.

Views sollen pro View mehrere Layer ein-/ausschalten koennen. Das Model soll eine dokumentweite Layerstruktur besitzen.

## Architekturvorschlag

### Core Layer-Strukturen

```cpp
using TDVecLayerId = std::string;

struct TDVecLayer {
    TDVecLayerId id;
    std::string name;
    bool visibleByDefault = true;
    bool locked = false;
    int order = 0;
};

struct TDVecObjectLayerRef {
    TDVecLayerId layerId;
    int orderInLayer = 0;
};
```

Integration in `TDVecObject`:

```cpp
class TDVecObject {
public:
    const TDVecLayerId& LayerId() const;
    void SetLayerId(TDVecLayerId layerId);

    int OrderInLayer() const;
    void SetOrderInLayer(int order);
};
```

Integration in `TDVecModel`:

```cpp
class TDVecModel {
public:
    const std::vector<TDVecLayer>& Layers() const;
    TDVecLayer* FindLayer(const TDVecLayerId& id);

    TDVecLayerId CreateLayer(std::string name);
    bool RemoveLayer(const TDVecLayerId& id);
    bool MoveObjectToLayer(const TDVecObjectId& objectId, const TDVecLayerId& layerId);
    bool SetObjectOrderInLayer(const TDVecObjectId& objectId, int order);
};
```

## View-spezifische Layer-Sichtbarkeit

Layer existieren im Model. Welche Layer eine konkrete View sieht, kann view-spezifisch sein.

```cpp
struct TDVecLayerViewState {
    TDVecLayerId layerId;
    bool visible = true;
};

struct TDVecViewLayerSettings {
    std::vector<TDVecLayerViewState> layers;
};
```

Damit kann eine View z.B. nur Konstruktion anzeigen, eine andere finale Zeichnung plus Annotationen.

## Zeichenreihenfolge

Empfohlene Reihenfolge:

1. Layer nach `TDVecLayer::order`.
2. Objekte innerhalb Layer nach `TDVecObject::OrderInLayer()`.
3. Bei gleicher Reihenfolge stable fallback auf Model-Einfuegereihenfolge oder ObjectId.

HitTesting sollte dieselbe Reihenfolge rueckwaerts verwenden:

- oberste sichtbare Objekte zuerst.

## UI-Ideen

- Layer-Dock:
  - Liste aller Layer
  - Sichtbarkeit toggle
  - Lock toggle
  - Reihenfolge hoch/runter
  - neuer Layer
  - Layer loeschen
- Object List:
  - Layer-Spalte
  - Objekt in anderen Layer verschieben
  - Reihenfolge innerhalb Layer anpassen
- Context Menu:
  - Bring Forward
  - Send Backward
  - Bring to Front in Layer
  - Send to Back in Layer
  - Move to Layer

## Persistenz

Layer gehoeren zum Dokument und muessen gespeichert werden:

- Layer IDs
- Namen
- Reihenfolge
- Default-Sichtbarkeit
- Lock-Status
- Objekt-Layer-Zuordnung
- Objekt-Reihenfolge innerhalb Layer

View-spezifische Layer-Sichtbarkeit kann optional separat gespeichert werden, ist aber nicht zwingend Dokumentinhalt.

## Plugin-Bezug

Plugins koennen Layer nutzen:

- UML-Plugin erzeugt Layer fuer Diagramm, Annotationen, Hilfslinien.
- Organigramm-Plugin trennt Boxen, Connectoren, Kommentare.
- Export-Plugins koennen Layer gezielt ausgeben oder filtern.

Plugins sollten aber nicht eigene Layer-Systeme bauen muessen.

## Abhaengigkeiten

Diese Story profitiert von:

- stabilen Object IDs,
- Object List Story,
- Persistenz Story,
- Multi-View Story fuer view-spezifische Layer-Sichtbarkeit.

## Akzeptanzkriterien

- Model enthaelt echte Layer.
- Jedes Objekt gehoert zu einem Layer.
- Objekte koennen zwischen Layern verschoben werden.
- Reihenfolge innerhalb eines Layers ist steuerbar.
- Rendering respektiert Layer- und Objekt-Reihenfolge.
- HitTesting respektiert sichtbare Layer und Reihenfolge.
- Views koennen Layer ein-/ausschalten.
- Core bleibt Qt-frei.
