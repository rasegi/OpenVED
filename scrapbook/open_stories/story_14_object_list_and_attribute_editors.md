# Story: Objektliste, Attributdialoge und Model-Metadaten-Editor

Datum: 2026-05-15

## Kontext

VED hat fachlich eine MVC-Struktur:

- Model enthaelt die Daten und Objekte.
- View stellt diese Daten grafisch dar.
- Operationen bearbeiten die Daten ueber Maus-/Tastaturinteraktion.

Neben der grafischen View soll es eine strukturierte Sicht auf die Model-Daten geben:

- zentrale Objektliste fuer alle Elemente im Model,
- hierarchische Darstellung des Model-Inhalts nach Layern,
- Objektattribute direkt ansehen und bearbeiten,
- eigene Dialoge/Editoren fuer einzelne Objekttypen,
- separater Dialog fuer Model-/Dokument-Metadaten.

Diese Story kann nach der Plugin-Grundlage angegangen werden, weil Plugins spaeter eigene Attribute/Metadaten beisteuern koennen.

Wichtige Abhaengigkeit:

- Diese Story haengt fachlich von `story_layers_and_object_order.md` ab.
- Die Objektliste soll nicht nur eine flache Objektliste sein, sondern das Innere des Models mit Layerstruktur zeigen.
- Deshalb sollte zuerst Layer und Objekt-Reihenfolge im Core eingefuehrt werden.

## Ziel

Eine nicht-grafische Model-Sicht einfuehren:

- Objektliste aller Model-Elemente.
- Darstellung als Baum:
  - Dokument/Model
  - Layer 1
  - Objekte in Layer 1
  - Layer 2
  - Objekte in Layer 2
  - usw.
- Auswahl in Objektliste synchronisiert mit grafischer View.
- Auswahl in grafischer View synchronisiert mit Objektliste.
- Bearbeitbare Attribute je Objekt.
- Zentraler Editor fuer Dokument-/Model-Metadaten.
- Erweiterbar fuer Plugin-Metadaten.

## Architekturvorschlag

### Voraussetzung: Layer im Core

Mindestens ein Layer muss immer existieren. Spaeter koennen mehrere Layer existieren.

Die Core-Methoden fuer Layer gehoeren in `TDVecModel` oder eine eng angebundene Model-Komponente.

Beispiel:

```cpp
class TDVecModel {
public:
    TDVecLayerId CreateLayer(std::string name);
    bool RemoveLayer(const TDVecLayerId& id);
    bool RenameLayer(const TDVecLayerId& id, std::string newName);
    bool MoveLayer(const TDVecLayerId& id, int newOrder);

    const std::vector<TDVecLayer>& Layers() const;
    TDVecLayer* FindLayer(const TDVecLayerId& id);
    const TDVecLayer* FindLayer(const TDVecLayerId& id) const;

    bool MoveObjectToLayer(const TDVecObjectId& objectId, const TDVecLayerId& layerId);
    std::vector<TDVecObject*> ObjectsInLayer(const TDVecLayerId& layerId);
    std::vector<const TDVecObject*> ObjectsInLayer(const TDVecLayerId& layerId) const;
};
```

Wenn noch kein expliziter Layer existiert, sollte das Model einen Default-Layer erzeugen, z.B.:

```cpp
TDVecLayer {
    id = "default";
    name = "Layer 1";
    visibleByDefault = true;
    locked = false;
    order = 0;
}
```

### Core

Der Core soll keine Qt-Dialoge kennen, aber introspektionsfaehige Daten bereitstellen.

Moegliche Core-Bausteine:

```cpp
struct TDVecObjectDescriptor {
    TDVecObjectId id;
    TDVecObjectType type;
    std::string displayName;
    TDVecLayerId layerId;
    bool selected = false;
    bool visible = true;
    int zOrder = 0;
};

struct TDVecLayerDescriptor {
    TDVecLayerId id;
    std::string name;
    bool visible = true;
    bool locked = false;
    int order = 0;
    std::vector<TDVecObjectDescriptor> objects;
};

class TDVecModel {
public:
    std::vector<TDVecObjectDescriptor> DescribeObjects() const;
    std::vector<TDVecLayerDescriptor> DescribeLayerTree() const;
    TDVecObject* FindObjectById(const TDVecObjectId& id);
    const TDVecObject* FindObjectById(const TDVecObjectId& id) const;
};
```

### Attribute Model

Um UI-unabhaengig Attribute bearbeiten zu koennen:

```cpp
enum class TDVecPropertyType {
    Bool,
    Integer,
    Double,
    String,
    Color,
    Enum,
    Point,
    Rect
};

struct TDVecPropertyDescriptor {
    std::string key;
    std::string displayName;
    TDVecPropertyType type;
    bool editable = true;
};

class IVecPropertyProvider {
public:
    virtual std::vector<TDVecPropertyDescriptor> PropertiesFor(const TDVecObject& object) const = 0;
    virtual TDVecPropertyValue GetProperty(const TDVecObject& object, std::string_view key) const = 0;
    virtual bool SetProperty(TDVecObject& object, std::string_view key, const TDVecPropertyValue& value) = 0;
};
```

Qt kann daraus Tabellen, Formularfelder und Dialoge bauen.

### Qt UI

Moegliche UI-Komponenten:

- `ObjectListDock`
  - Baum aller Layer und Objekte.
  - Root: Model/Dokument.
  - Kind-Ebene 1: Layer.
  - Kind-Ebene 2: Objekte innerhalb des Layers.
  - Spalten: Name, Typ, ID, Sichtbarkeit, Lock, Reihenfolge, Auswahl.
  - Layer-Zeilen erlauben Layer-Aktionen.
  - Objekt-Zeilen erlauben Objekt-Aktionen.
- `ObjectPropertiesDock`
  - Property-Grid fuer aktuelles Objekt.
  - Bei mehreren Objekten gemeinsame Attribute anzeigen.
- `LayerPropertiesDialog`
  - Name, Sichtbarkeit, Lock-Status, Reihenfolge.
  - spaeter ggf. Farbe/Default-Style des Layers.
- `ObjectDetailDialog`
  - Spezifischer Dialog je Objekttyp, z.B. Line, Polygon, Ellipse, Group.
- `DocumentMetadataDialog`
  - Dokumentweite Settings und Metadata.
  - spaeter inklusive Plugin-Metadaten.

## Plugin-Bezug

Plugins koennen spaeter:

- eigene Layer-Knoten oder virtuelle Gruppierungen anbieten, falls sinnvoll,
- eigene Spalten fuer die Objektliste liefern,
- eigene PropertyProvider registrieren,
- eigene Detaildialoge bereitstellen,
- eigene Model-/Dokument-Metadaten sichtbar machen.

Beispiel:

- Organigramm-Plugin zeigt Spalte `Rolle`.
- UML-Plugin zeigt Spalte `Elementtyp`.
- Decorate-Plugin zeigt Pfeil-/Linienmarkerattribute.

## Akzeptanzkriterien

- `story_layers_and_object_order.md` ist umgesetzt oder mindestens die noetigen Core-Layer-APIs sind vorhanden.
- Alle Model-Objekte sind in einer zentralen Liste sichtbar.
- Die Liste zeigt mindestens einen Layer und dessen Objekte.
- Bei mehreren Layern zeigt die Liste alle Layer mit ihren zugeordneten Objekten.
- Layer koennen aus der UI erzeugt, umbenannt und in ihrer Reihenfolge verwaltet werden, sofern die Layer-Core-Story umgesetzt ist.
- Selektion ist zwischen Liste und grafischer View synchron.
- Attribute eines Objekts koennen angezeigt und bearbeitet werden.
- Attribute eines Layers koennen angezeigt und bearbeitet werden.
- Model-/Dokument-Metadaten koennen angezeigt werden.
- Core bleibt Qt-frei.
- Plugin-Metadaten koennen spaeter integriert werden, ohne das Konzept neu zu bauen.
