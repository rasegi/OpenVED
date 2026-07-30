# Konzept: Plugin- und Modul-Architektur fuer VED

Datum: 2026-07-30
Status: offen (Konzept / Analyse — keine Umsetzung)

## Zweck

Dieses Papier fuehrt zwei bisher getrennte Straenge zu **einer** Plugin-Architektur
zusammen:

- **`story_plugin_architecture_persistent_semantics.md`** — semantische Dokument-
  Plugins (Organigramm/UML), persistente Metadaten, Behavior-Hooks, Qt-Huelle.
- **`story_16`-Nachfolge / "Story 18"-Idee** — Ausgabe-Funktionen (Print, PDF-,
  DXF-Export, spaeter Operationen) als **entkoppelte Module/Plugins**. (Papier
  bleibt dagegen eine **feste Model-Eigenschaft**, kein Plugin — siehe Abschnitt 7.)

Es trifft die **Technologie-Entscheidung** (CPPMicroServices als OSGi-artiger
Service-Layer, mit optionaler Qt-Plugin-Huelle) und umreisst die **ersten
Refaktorisierungs-Stories**. Konkrete Interfaces/Signaturen hier sind
illustrativ — verbindlich wird erst die jeweilige Umsetzungs-Story.

## 1. Vision & Leitprinzipien

VED soll vom Vektor-Editor zu einem **erweiterbaren Framework fuer fachliche
Editoren** werden. Leitprinzipien:

1. **Core bleibt Qt-frei** (harter Architektur-Grundsatz). Die Service-Registry
   und alle Core-Plugin-Teile kennen keine Qt-Typen.
2. **Ein Codemodell, zwei Deployments:** nativ koennen Plugins dynamisch geladen
   werden, unter **WebAssembly** werden dieselben Plugins **statisch** einkompiliert
   und nur zur Laufzeit ueber die Registry aktiviert (WASM kennt kein `dlopen`/
   `QPluginLoader` von `.dll`/`.so`).
3. **Optionalitaet:** Erweiterungen (Diagramm-Semantik, Meshing, Exporter) ergeben
   nicht in jedem Kontext Sinn und muessen als Plugins zu-/abschaltbar sein. Das
   betrifft **Funktionen**, **nicht** Model-Kern-Eigenschaften wie Einheit oder
   Papier — die bleiben fest.
4. **Deklarative Abhaengigkeiten** zwischen Plugins (Capability/Requirement),
   azyklisch aufgeloest — nicht harte C++-Klassenkopplung.
5. **Persistenz-Vertraeglichkeit:** Unbekannte Plugin-Daten muessen ladbar bleiben
   und beim Speichern erhalten werden (Roundtrip).

### 1.1 Ausbaurichtungen (Produkt-Vision)

OpenVED ist heute ein rudimentaerer **2D-Vektor-Editor**. Er ist die gemeinsame
Basis; die eigentlichen Produkte entstehen als **Plugin-Familien** darauf. Genau
diese Vielfalt begruendet die Investition in eine echte Plugin-Architektur:

| Richtung | Beschreibung | Plugin-Einordnung | Abhaengigkeiten |
|---|---|---|---|
| **A — Diagramm-Editoren** | Organigramm, UML u.ae. | **Semantic/Model-Plugins**: eigene Object-Types (Box/Connector/Klasse), Operationen, Behaviors, Metadaten | Kern-Object-Types; Behavior-Hooks; Metadaten-Persistenz |
| **B — Bild → Mesh** | Foto (z.B. Gesicht) → trianguliertes Model, 2D und/oder 3D, optional OpenCascade | **Feature-Plugin** (Bild-Import + Meshing-Service) + erzeugt Object-Types (Triangulation/Mesh) | 3D-Object-Types (Richtung C); **OpenCascade** als optionale externe Capability |
| **C — 3D-Vektor-Editor** | Editor fuer 3D-Vektorobjekte | **tiefste Erweiterung**: 3D-Object-Types + 3D-Rendering + 3D-Operationen | ein 3D-WorkingSpace kennt kein Blattformat; Print/Export verhalten sich anders als in 2D |
| **D — Editor-Bausteine** | Layer, dekorative Attribute (Linienstaerke/-art/-farbe), Listen-/Eigenschaften-Editoren, **DXF-Export** | teils **Feature-Capabilities** (DXF = Exporter wie PDF), teils **UI-Capabilities** (Property/List-Editoren), teils Core-Erweiterung (Layer/Attribute) | Exporter-Registry; UI-Registry; Core-Metadaten |

**Beobachtungen fuer die Architektur:**
- **B haengt an C** (Mesh-Plugin braucht die 3D-Object-Types) — ein konkreter Fall des
  Capability/Requirement-Resolvers (Abschnitt 5).
- **OpenCascade** ist eine schwergewichtige, optionale externe Abhaengigkeit — ideal
  als eigenes Bundle, das nur bei Bedarf mitgebaut/aktiviert wird (WASM-Tauglichkeit
  gesondert zu pruefen).
- **DXF-Export** wird der **zweite Provider** derselben Exporter-Capability wie
  PDF — validiert die Abstraktion aus Fahrplan-Story D.
- **Richtung D laeuft teilweise schon** als Kern-Stories (`story_11_layers_and_object_order`,
  `story_12_core_decorative_attributes`, `story_14_object_list_and_attribute_editors`);
  diese wandern perspektivisch hinter Capability-/UI-Interfaces und werden spaeter
  als Plugins ausgeliefert.

## 2. Plugin-Typen — Taxonomie

Die bestehende Story kennt faktisch nur *einen* Typ (semantische Plugins). Die
zusammengefuehrte Architektur braucht eine **Schichtung**:

| Typ | Liefert | Beispiele |
|---|---|---|
| **Object-Type-Provider** | Was ein Objekt *ist*: Geometrie, Rendering, Serialisierung | Kern: Linie/Kreis/Text; Plugin: Organigramm-Box |
| **Operation-Provider** | *Werkzeug*, das Objekte erzeugt/bearbeitet (`voc_*`/`vom_*`) | Kern: Select/Move; Plugin: "Connector zeichnen" |
| **Feature/Capability-Provider** | Querschnitts-Dienst / Ausgabe | **Print**, **PDF-Export**, **DXF-Export**, Grid, Rulers |
| **Semantic/Model-Plugin** | *Aggregat*, das eigene Object-Types + Operations + Behaviors + Metadaten buendelt | Organigramm, UML |

**Wichtig:** Ein Semantic/Model-Plugin ist die unterste Zeile — es *bringt seine
eigenen Object-Type- und Operation-Provider mit* (der ausdrueckliche Wunsch:
"ein Model-Plugin bringt seine Operationen mit").

## 3. Technologie-Entscheidung

### 3.1 CPPMicroServices als Service-Layer (Empfehlung)

[CPPMicroServices](https://github.com/CppMicroServices/CppMicroServices) ist "an
OSGi-like C++ dynamic module system and service registry".

- **Lizenz:** Apache-2.0 (compliance-freundlich, passt zu `THIRD_PARTY_LICENSES.md`).
- **Sprache:** C++17 (VED ist C++23 → kompatibel).
- **Service-Registry:** `BundleContext`, `RegisterService<I>(impl)` /
  `GetServiceReference<I>()` + `GetService()`, Service-Properties fuer Lookup,
  `BundleActivator::Start/Stop`-Lifecycle. Deckt genau das Capability/Service-Modell
  ab, das wir brauchen.
- **Static Bundles (der WASM-Schluessel):** Bundles koennen **statisch** einkompiliert
  werden (`CPPMICROSERVICES_IMPORT_BUNDLE`), **API-identisch** zu shared bundles.
  Nativ dynamisch, WASM statisch — **derselbe Plugin-Code**. Manifest (`manifest.json`)
  + Ressourcen werden per `usResourceCompiler3` in die Library eingebettet.

**Warum das passt:** Der Registry-/Bundle-Kern ist Qt-frei → er lebt in `ved_core`
(bzw. einem neuen `ved_plugin_host`-Modul) und verletzt den Architektur-Grundsatz
nicht.

### 3.2 Zwei Ebenen: mit und ohne Qt-Huelle

Analog zur bestehenden Plugin-Story, jetzt praezisiert:

- **Core-Bundle (immer):** Qt-frei, registriert Capabilities (Object-Types,
  Operations, Exporter) ueber CPPMicroServices. Headless nutzbar (CLI,
  Tests, Server).
- **Qt-Plugin-Huelle (optional):** liefert UI (Actions, Toolbars, DockWidgets,
  Dialoge) ueber `QPluginLoader` + ein Qt-nahes Interface (`IVecQtPlugin` im Modul
  `ved_qt_plugin_api`). Verbindet sich mit dem Core-Bundle desselben Plugins.

Ein Bundle **kann nur einen Core-Teil** haben (z.B. ein reiner PDF-Exporter,
headless) — dann "ohne qt-plugin". Ein UI-Werkzeug hat **beide** Ebenen.

### 3.3 Abgewogene Alternativen

| Option | Pro | Contra |
|---|---|---|
| **CPPMicroServices** (Empfehlung) | OSGi-Modell fertig; static+dynamic API-gleich; Service-Registry + Lifecycle; Qt-frei; Apache-2.0 | Zusaetzliche Dependency; Lernkurve; **WASM/Emscripten nicht offiziell dokumentiert** (Spike noetig) |
| **Reines Qt-Plugin-System** (`QPluginLoader`) | schon via Qt vorhanden | dynamisch → **WASM-Problem**; keine Service-Registry/Dependency-Resolution/Versionierung; droht Core mit Qt zu koppeln |
| **Eigenbau-Registry (OSGi-Stil, schlank)** | volle Kontrolle; minimal; Qt-frei; WASM = nur statische Registrierung | wir bauen OSGi-Konzepte (Resolver, Versionierung, Lifecycle) selbst nach |

**Richtung:** CPPMicroServices — **vorbehaltlich** des WASM-Spikes (3.4). Faellt der
negativ aus, ist der **schlanke Eigenbau** der Fallback (das Capability/Requirement-
Modell aus Abschnitt 5 ist framework-unabhaengig formuliert).

### 3.4 WASM — kritischer Vorab-Spike

Der README nennt **weder Emscripten noch WebAssembly**. Der Static-Bundle-Mechanismus
ist der plausible Weg, aber **unverifiziert**. Vor jeder Festlegung: ein Spike, der
**ein triviales static Bundle nativ UND unter Emscripten** baut, aktiviert und einen
Service aufloest. Ergebnis entscheidet CPPMicroServices vs. Eigenbau.

## 4. Architektur-Schichten

```text
ved_core                      (Qt-frei)
  TDVecModel / TDVecObject
  TDVecMetadataBag            (persistente Plugin-Metadaten)
  Object-IDs                  (stabile Identitaet)
  Core-Capability-Interfaces  (Object-Type, Operation, Exporter ...)

ved_plugin_host               (Qt-frei; CPPMicroServices-Wrapper ODER Eigenbau)
  Service-Registry / BundleContext
  Capability/Requirement-Resolver (azyklisch, versioniert)

ved_qt_plugin_api             (Qt-nah)
  IVecQtPlugin, UI-Registry (Actions/Toolbars/Docks)

ved_qt_app
  PluginManager (Qt-Huellen laden) + Bindung an ved_plugin_host

bundles/*                     (je Plugin)
  Core-Teil  (Capabilities, Qt-frei)
  Qt-Huelle  (optional)
  manifest.json
```

## 5. Dependency-Modell (Capability / Requirement)

Der Kern der "Story 18"-Frage: *Operation-Plugin "X-erzeugen" braucht X entweder im
Kern oder per Plugin injiziert.* Loesung im OSGi-Stil:

- **Deklarativ, nicht hart verlinkt:** Ein Bundle deklariert im Manifest
  `Requires: object-type "connector" (>=1.0)` und `Provides: operation "draw-connector"`.
  Es haengt an einer **Capability**, nicht an einer C++-Klasse.
- **Kern als System-Bundle:** Der Kern registriert seine Basis-Object-Types als
  Capabilities. Damit ist "im Kern vorhanden" und "per Plugin geliefert" **derselbe
  Mechanismus** — nur eine andere Quelle.
- **Resolver:** Beim Start werden Requirements gegen Provides aufgeloest,
  **azyklisch** (topologische Sortierung → Aktivierungsreihenfolge). Ein Zyklus ist
  ein Resolve-Fehler, kein Laufzeitproblem.
- **Versionierung:** semver-Ranges an den Requirements.

## 6. Persistente Semantik & Metadaten

Uebernommen aus `story_plugin_architecture_persistent_semantics.md` (dort im Detail):

- `TDVecMetadataBag` an Model **und** Objekt; Eintraege mit `pluginId`, `schemaId`,
  `schemaVersion`, `payload`.
- **Unknown-Roundtrip:** Dokument mit Metadaten eines nicht installierten Plugins
  bleibt ladbar; Metadaten gehen beim Speichern nicht verloren; UI darf warnen.
- **Object-IDs** als Voraussetzung fuer persistente Beziehungen (Connector from/to).
- Behavior-Hooks (z.B. Move) fuer semantische Erweiterungen.

Dieses Papier aendert daran nichts — es ordnet es als **Persistenz-Fundament** unter
die gemeinsame Registry ein.

## 7. Fallstudie: Print / PDF / DXF-Export (Exporter-Plugins)

**Papier ist KEIN Plugin.** Es ist eine **feste Eigenschaft des Model/WorkingSpace**
(das Feld `TDVecPageSettings` in `TDVecDocumentSettings`, `vec_document_settings.h`)
— wie Einheit und Grid, ohne eigene Klasse/Datei. Es bleibt fester Bestandteil und
wird **nicht** entkoppelt oder optional gemacht.

Zu Plugins werden nur die **Ausgabe-Funktionen**, die das Papier *konsumieren*:
**Print**, **PDF-Export** und (Richtung D) **DXF-Export**.

**Heutige Kopplung (Ist-Analyse):**
- Print (`MainWindow.cpp:1018ff`) und PDF-Export (`:952ff`) sind **fest im
  App-Layer verdrahtet** und lesen `ds.pageSettings` direkt als (a) Ausgabegroesse
  (`QPageSize`) und (b) Rendering-Ausschnitt (Papierrechteck) — plus den
  `TDVecExportCoordinateMapper` (Core) fuer Real→mm→Points.

**Zielbild:** Print/PDF/DXF je als eigenes Exporter-Bundle hinter einer gemeinsamen
`IVecExporter`-Capability. `MainWindow` ruft nicht mehr direkt `QPdfWriter`/`QPrinter`,
sondern loest den Exporter ueber die Registry auf. Die Exporter **lesen** das (feste)
Papier + die Objekte aus dem Model — Papiergroesse/-format bleiben die Quelle des
Ausgabe-Rahmens, nur der *Weg dorthin* wird ein Plugin.

Dieser Schnitt ist der **beste erste Feature-Plugin-PoC**, weil der Export bereits
relativ isoliert ist (`vec_export_coordinate_mapper`) und **keine** Model-Kern-Aenderung
braucht.

## 8. 3D-Ausblick

Papier bleibt eine **feste** Eigenschaft des heutigen **2D**-WorkingSpace. Ein
spaeterer **3D**-WorkingSpace kennt kein Blattformat — dort waere "Print"/Ausgabe
eine andere Capability (View-Snapshot/Render). Genau deshalb sollen die
**Ausgabe-Funktionen** (Print/PDF/Export) Plugins sein: sie duerfen je nach
Dimensionalitaet unterschiedlich ausfallen, ohne den Model-Kern anzufassen. Der
Model-Kern (WorkingSpace + Einheit + im 2D-Fall Papier) bleibt fest.

## 9. Grober Fahrplan — erste Refaktorisierungs-Stories

Reihenfolge nach Risiko (risikoarme Fundamente zuerst), praktisch als Refactors der
bestehenden Codebasis:

1. **Story A — Object-IDs im Core.** Stabile `TDVecObjectId`; Vergabe beim Erzeugen,
   Erhalt beim Speichern/Laden. Voraussetzung fuer alles Persistente.
2. **Story B — MetadataBag + Unknown-Roundtrip.** `TDVecMetadataBag` an Model/Objekt;
   Dateiformat um plugin-fremde Metadaten erweitern (roundtrip-fest, versioniert).
3. **Story C — Registry-Fundament + WASM-Spike.** Trivial-Bundle nativ **und** WASM
   (static) bauen/aktivieren/aufloesen. Entscheidung CPPMicroServices vs. Eigenbau.
   Neues Modul `ved_plugin_host` (Qt-frei).
4. **Story D — Export als erstes Capability-Bundle.** PDF-Export headless (ohne
   Qt-Huelle) hinter eine `IVecExporter`-Capability ziehen; `MainWindow` konsumiert
   nur noch die Registry. **DXF-Export** (Richtung D) wird spaeter der zweite
   Provider derselben Capability und validiert die Abstraktion.
5. **Story E — Print & DXF als weitere Exporter-Plugins.** Print und DXF-Export ueber
   dieselbe `IVecExporter`-Capability wie PDF (Story D); validiert Multi-Provider. Alle
   lesen das **feste** Papier aus dem Model — Papier wird nicht entkoppelt.
6. **Story F — Qt-Plugin-Huelle.** `ved_qt_plugin_api` + `PluginManager`; erstes
   UI-Bundle ueber `QPluginLoader` (nativ) bzw. statisch registriert (WASM).
7. **Story G (PoC) — domaenenspezifisches Model-Plugin.** Mini-Decorate/Organigramm:
   bringt eigenen Object-Type + Operation + Metadaten mit; testet Dependency-Resolution
   (Story-C-Resolver) und Behavior-Hooks.

Operationen (`voc_*`/`vom_*`) werden **nicht pauschal** zu Plugins: Basis-Werkzeugkasten
(Select/Move/Delete + Grundprimitive) bleibt **Kern/System-Bundle**; nur
domaenenspezifische Operationen werden Plugins (Story G aufwaerts). Grund: die
heutigen Operationen sind interaktive Zustandsautomaten, eng an
Model/EditCad/OperationManager — pauschale Extraktion waere teuer und riskant.

## 10. Risiken & offene Fragen

- **CPPMicroServices unter WASM/Emscripten unbestaetigt** → Story C entscheidet
  (Fallback: Eigenbau).
- **Zu frueh = Portierung ausbremsen** (die bestehende Story warnt zu Recht). Deshalb
  Fundamente (A/B) zuerst, grosser Umbau spaeter.
- **Operationen-Extraktion** ist die teuerste offene Frage (interaktive Automaten).
- **Schema-Versionierung/Migration** der Plugin-Metadaten.

## 11. Bezug zu bestehenden Stories

- **`story_plugin_architecture_persistent_semantics.md`** — liefert Persistenz/
  Metadaten/Behavior-Bausteine (Abschnitt 6); wird von diesem Konzept als Fundament
  unter die gemeinsame Registry gestellt (Stories A/B).
- **`story_16_webassembly.md`** — begruendet die WASM-Constraints (static bundles,
  kein `dlopen`); Story C haengt am WASM-Setup aus Story 16.
- **`story_font_provider_truetype_converter_plugin.md`** — ein frueher Provider-/
  Plugin-Kandidat; passt in dasselbe Capability-Modell (`IVecFontProvider` als
  Service).
- **`closed_stories/concept_units_pages_templates_pdf_print.md`** — Ist-Design von
  Papier/Einheiten/PDF/Print, Basis fuer die Fallstudie (Abschnitt 7).
- **`story_11_layers_and_object_order.md`, `story_12_core_decorative_attributes.md`,
  `story_14_object_list_and_attribute_editors.md`** — laufende Kern-Stories der
  Ausbaurichtung D (Abschnitt 1.1); wandern perspektivisch hinter Capability-/UI-
  Interfaces und werden spaeter als Plugins ausgeliefert.
