# Story: Plugin Architecture mit persistenter Semantik und Qt-Huelle

Datum: 2026-05-15

## Kontext

VED soll langfristig nicht nur ein Vektor-Editor sein, sondern auch als erweiterbares Framework fuer fachliche Editoren dienen koennen.

Beispiele:

- Organigramm-Plugin:
  - Boxen sind nicht nur Polygone/Rechtecke, sondern "Stellen" oder "Organisationseinheiten".
  - Verbindungslinien sind fachliche Beziehungen.
  - Beim Verschieben einer Box sollen verbundene Linien automatisch mitgefuehrt und neu justiert werden.
- UML-Plugin:
  - Gruppen von grafischen Objekten repraesentieren Klassen, Interfaces, Beziehungen.
  - Linien haben Rollen wie Aggregation, Komposition, Vererbung.
- Decorate-Plugin:
  - Linien koennen Pfeile, Start-/Endmarker oder semantische Rollen bekommen.
  - Gruppen koennen als Haus, Box, Container, Diagramm-Element usw. markiert werden.

Diese Zusatzinformationen sind nicht nur Exportdaten. Sie gehoeren zum Dokument und muessen persistiert werden, auch wenn der Core sie fachlich nicht vollstaendig versteht.

## Ziel

Ein Plugin-System entwerfen, das:

- aus der Qt-UI heraus ladbar und bedienbar ist,
- eine Qt-Huelle fuer UI, Actions, Toolbars, DockWidgets und Dialoge erlaubt,
- optional Core-/Semantik-Erweiterungen bereitstellt,
- persistente Metadaten am Model und an Objekten erlaubt,
- das native VED-Dokumentformat plugin-erweiterbar macht,
- den VED-Kern nicht mit Qt-Abhaengigkeiten verseucht.

## Grundsatz

Jedes Plugin wird praktisch aus der UI heraus geladen und aktiviert. Deshalb braucht jedes Plugin eine Qt-nahe Huelle.

Diese Qt-Huelle darf aber nicht bedeuten, dass der VED-Core Qt-Typen kennen muss.

Stattdessen:

- Qt-App/Qt-Plugin-Layer laedt Plugins.
- Das Plugin kann UI-Teile bereitstellen.
- Das Plugin kann zusaetzlich einen Core-/Semantik-Teil bereitstellen.
- Der Core-/Semantik-Teil spricht nur ueber Qt-freie VED-Core-Interfaces mit Model, Operationen, Persistenz und Metadaten.

## Architekturvorschlag

```text
ved_core
  TDVecModel
  TDVecObject
  TDVecMetadataBag
  TDVecDocumentSerializer
  TDVecCorePlugin Interfaces
  TDVecSemanticBehavior Interfaces

ved_qt_app
  MainWindow
  QVedWidget
  PluginManager
  QtPluginLoader
  Toolbar/Menu/Dock Integration

plugin_xyz
  Qt-Huelle
    Actions
    Icons
    Toolbars
    DockWidgets
    Dialoge
  Core-/Semantik-Teil
    Metadata Schema
    Behavior Hooks
    Export/Import
    Validation/Migration
```

## Zwei Ebenen eines Plugins

### 1. Qt Plugin Facade

Die Qt-Huelle ist fuer Laden, UI und Bedienung verantwortlich.

Sie darf Qt verwenden:

- `QObject`
- `QAction`
- `QIcon`
- `QDockWidget`
- `QWidget`
- `QPluginLoader`

Beispiel-Interface im Qt-Layer:

```cpp
class IVecQtPlugin {
public:
    virtual ~IVecQtPlugin() = default;

    virtual std::string Id() const = 0;
    virtual std::string DisplayName() const = 0;
    virtual std::string Version() const = 0;

    virtual void RegisterUi(IVecQtPluginUiRegistry& registry) = 0;
    virtual IVecCorePlugin* CorePlugin() = 0;
};
```

`IVecQtPlugin` liegt nicht in `ved_core`, sondern in einem Qt-nahen Modul, z.B. `ved_qt_plugin_api`.

### 2. Core Plugin / Semantic Extension

Der Core-Teil ist Qt-frei und arbeitet nur mit VED-Core-Typen.

Beispiel:

```cpp
class IVecCorePlugin {
public:
    virtual ~IVecCorePlugin() = default;

    virtual std::string Id() const = 0;
    virtual std::string Version() const = 0;

    virtual void RegisterMetadataSchemas(IVecMetadataRegistry& registry) = 0;
    virtual void RegisterBehaviors(IVecBehaviorRegistry& registry) = 0;
    virtual void RegisterImportExport(IVecFormatRegistry& registry) = 0;
};
```

Der Core kennt nur dieses Interface. Keine Qt-Typen.

## Persistente Metadaten

VED-Objekte und das Dokument selbst brauchen plugin-erweiterbare Metadaten.

### Metadata Bag

```cpp
struct TDVecMetadataEntry {
    std::string pluginId;
    std::string schemaId;
    uint32_t schemaVersion = 1;
    std::vector<std::byte> payload;
};

class TDVecMetadataBag {
public:
    void AddOrReplace(TDVecMetadataEntry entry);
    std::vector<TDVecMetadataEntry> EntriesForPlugin(std::string_view pluginId) const;
    const std::vector<TDVecMetadataEntry>& Entries() const;

private:
    std::vector<TDVecMetadataEntry> entries_;
};
```

Integration:

```cpp
class TDVecObject {
public:
    TDVecMetadataBag& Metadata();
    const TDVecMetadataBag& Metadata() const;
};

class TDVecModel {
public:
    TDVecMetadataBag& DocumentMetadata();
    const TDVecMetadataBag& DocumentMetadata() const;
};
```

### Wichtige Regel

Der Core muss unbekannte Metadaten erhalten koennen.

Wenn ein Dokument Metadaten von `ved.organigram` enthaelt, das Plugin aber nicht installiert ist:

- Dokument muss ladbar bleiben.
- Metadaten duerfen beim Speichern nicht verloren gehen.
- UI darf warnen, dass semantische Bearbeitung fuer dieses Plugin nicht verfuegbar ist.
- Normale Vektorobjekte bleiben weiterhin sichtbar und bearbeitbar.

## Semantische Verhaltenserweiterungen

Plugins sollen nicht nur Daten speichern, sondern auch Verhalten erweitern koennen.

Beispiel Organigramm:

- Eine Box ist eine Stelle.
- Verbindungslinien haengen semantisch an dieser Box.
- Beim Move einer Box sollen Linien mitbewegt oder neu an Attach-Points gefuehrt werden.

Dafuer braucht es Hook-/Behavior-Schnittstellen.

### Vorschlag: Operation Hooks

```cpp
struct TDVecObjectMoveContext {
    TDVecModel& model;
    TDVecObjectId objectId;
    double dx;
    double dy;
};

class IVecObjectMoveBehavior {
public:
    virtual ~IVecObjectMoveBehavior() = default;

    virtual bool WantsObjectMove(const TDVecModel& model, TDVecObjectId objectId) const = 0;
    virtual void BeforeObjectMove(TDVecObjectMoveContext& context) = 0;
    virtual void AfterObjectMove(TDVecObjectMoveContext& context) = 0;
};
```

Anwendung:

- Core-Operation `VOM_MOVE_OBJECT` bewegt weiter normale VED-Objekte.
- Vor/nach dem Move fragt der Core eine BehaviorRegistry.
- Ein Organigramm-Plugin kann reagieren:
  - verbundene Linien finden,
  - Attach-Points neu berechnen,
  - Linienendpunkte nachziehen,
  - ggf. Layout-Regeln anwenden.

### Alternative: Command/Transaction Hooks

Langfristig waere ein allgemeinerer Ansatz robuster:

```cpp
class TDVecCommand {
public:
    virtual void Execute(TDVecModel& model) = 0;
    virtual void Undo(TDVecModel& model) = 0;
};

class IVecCommandInterceptor {
public:
    virtual bool CanHandle(const TDVecCommand& command, const TDVecModel& model) const = 0;
    virtual void BeforeExecute(TDVecCommand& command, TDVecModel& model) = 0;
    virtual void AfterExecute(TDVecCommand& command, TDVecModel& model) = 0;
};
```

Das waere spaeter sinnvoll, wenn Undo/Redo und Transaktionen stabil sind.

## Object IDs als Voraussetzung

Persistente Semantik braucht stabile Objektidentitaeten.

Wenn ein Plugin Metadaten speichern will wie:

- Box `A`
- Linie `L1` verbindet Box `A` mit Box `B`

dann reichen rohe Pointer oder Listen-Indizes nicht.

Vorschlag:

```cpp
using TDVecObjectId = std::string;

class TDVecObject {
public:
    const TDVecObjectId& Id() const;
    void SetId(TDVecObjectId id);
};
```

Der Core muss IDs beim Erzeugen vergeben und beim Speichern/Laden erhalten.

## Native Persistenz mit Plugin-Erweiterungen

Das native VED-Dokumentformat bleibt Core-Verantwortung, muss aber plugin-erweiterbar sein.

Beispielstruktur:

```json
{
  "vedVersion": 1,
  "documentSettings": {},
  "documentMetadata": [
    {
      "pluginId": "ved.organigram",
      "schemaId": "document",
      "schemaVersion": 1,
      "payload": {}
    }
  ],
  "objects": [
    {
      "id": "obj-001",
      "type": "polygonArea",
      "geometry": {},
      "metadata": [
        {
          "pluginId": "ved.organigram",
          "schemaId": "org-box",
          "schemaVersion": 1,
          "payload": {
            "kind": "position",
            "title": "Teamleitung"
          }
        }
      ]
    },
    {
      "id": "obj-002",
      "type": "line",
      "geometry": {},
      "metadata": [
        {
          "pluginId": "ved.organigram",
          "schemaId": "connector",
          "schemaVersion": 1,
          "payload": {
            "from": "obj-001",
            "to": "obj-003",
            "fromAnchor": "bottom",
            "toAnchor": "top"
          }
        }
      ]
    }
  ]
}
```

Der Core muss:

- bekannte VED-Geometrie laden,
- unbekannte Plugin-Metadaten roundtrippen,
- bekannte Plugin-Metadaten optional validieren/migrieren lassen, wenn Plugin vorhanden ist.

## Plugin Lifecycle

Vorschlag:

1. Qt-App startet.
2. Qt Plugin Manager scannt Plugin-Verzeichnisse.
3. Qt-Huellen werden geladen.
4. Jede Qt-Huelle registriert UI-Beitraege.
5. Falls vorhanden, registriert sie ihren CorePlugin-Teil beim Core Plugin Manager.
6. Beim Dokument-Laden:
   - Core liest Geometrie und Metadaten.
   - Core fragt registrierte Plugins nach Schema-Migration/Validation.
   - Fehlende Plugin-IDs werden als "unknown extension data" markiert.
7. Beim Bearbeiten:
   - Operationen fragen BehaviorRegistry fuer semantische Erweiterungen.
8. Beim Speichern:
   - Core speichert bekannte und unbekannte Metadaten.

## Beispiel: Organigramm Move

Ausgang:

- Box `obj-box-1` hat Metadata:
  - pluginId: `ved.organigram`
  - schemaId: `org-box`
- Linie `obj-line-1` hat Metadata:
  - schemaId: `connector`
  - from: `obj-box-1`
  - to: `obj-box-2`

Aktion:

- User verschiebt Box `obj-box-1`.

Core-Ablauf:

1. `VOM_MOVE_OBJECT` erstellt Move-Kontext.
2. BehaviorRegistry findet `ved.organigram` als interessiert.
3. Core bewegt Box normal.
4. Organigramm-Behavior sucht Connector-Linien mit `from/to == obj-box-1`.
5. Plugin berechnet neue Attach-Points.
6. Plugin passt Linienendpunkte an.
7. Alles passiert in einer spaeteren Transaktion/Undo-Einheit.

Qt-Ablauf:

- Qt-Plugin liefert Toolbar/Inspector fuer Organigramm.
- Der eigentliche Move-Hook bleibt Qt-frei im Core-Teil.

## Risiken

- Zu fruehes Plugin-System kann die Portierung verlangsamen.
- Zu spaetes Plugin-System kann das native Dateiformat zu eng machen.
- Behavior-Hooks koennen Operationen komplizierter machen.
- Ohne stabile Object IDs sind persistente Beziehungen fragil.
- Plugin-Metadaten brauchen Schema-Versionierung und Migration.

## Empfehlung

Plugin-System nicht sofort voll implementieren, aber die Persistenz so planen, dass es nicht blockiert wird.

Nahe Zukunft nach Kern-Portierung:

1. Object IDs im Core einfuehren.
2. MetadataBag fuer Model und Objekte einfuehren.
3. Native Persistenz mit unknown metadata roundtrip planen.
4. Qt-freie Core-Plugin-Interfaces entwerfen.
5. Qt-Plugin-Huelle separat entwerfen.
6. Erstes Proof-of-Concept:
   - kleines Decorate-Plugin oder Organigramm-Miniplugin.
   - schreibt Metadata.
   - beeinflusst Move-Verhalten einer Box/Connector-Linie.

## Akzeptanzkriterien

- VED-Core bleibt Qt-frei.
- Plugins sind aus der Qt-UI heraus ladbar.
- Ein Plugin kann UI und Core-Semantik kombinieren.
- Plugin-Metadaten werden am Dokument und an Objekten persistiert.
- Unbekannte Plugin-Metadaten bleiben beim Laden/Speichern erhalten.
- Semantische Plugins koennen Operationen wie Move fachlich erweitern.
- Export-/Import-Plugins koennen dieselbe Plugin-Infrastruktur nutzen.
