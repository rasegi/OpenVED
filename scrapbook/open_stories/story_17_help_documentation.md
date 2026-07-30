# Story 17: Benutzer-Hilfe & Dokumentation (In-App + Web, kontextsensitiv)

Datum: 2026-07-30
Status: offen

## Ziel

OpenVED bekommt eine **Benutzer-Dokumentation**, erreichbar
- **in der App** ueber ein **Hilfe-Menue** (eingebettetes Fenster), und
- **kontextsensitiv per F1**: waehrend eine Operation aktiv ist, oeffnet F1 die
  Hilfe **nur fuer genau diese Operation**, und
- **im Web** (dieselben HTML-Seiten zusaetzlich auf GitHub Pages unter `/help/`).

## Architektur-Grundsatz

- **Eine HTML-Datei pro Operation/Werkzeug** (`help/ops/voc_line.html`, …) —
  modular, einzeln per F1 aufrufbar **und** ueber ein Inhaltsverzeichnis
  (`help/index.html`) zur Gesamt-Hilfe „zusammengesteckt".
- **Anzeige:** eingebettetes **`QTextBrowser`** (HTML-Subset + Bilder aus Qt-
  Ressourcen). Funktioniert **nativ und in WASM**, offline. (`QWebEngineView`
  scheidet aus — in WASM nicht verfuegbar.)
- **Eine Quelle, zwei Zugaenge:** die HTML unter `src/app/resources/help/` als
  Qt-Ressource einbetten (App) **und** dieselben Dateien auf GitHub Pages
  veroeffentlichen (Web, `/help/`).
- **Kontext-Mapping:** jede Operation liefert eine `helpId` (= Basisname, z. B.
  `voc_line`). `ved_core` bleibt Qt-frei — das Mapping `helpId → HTML` und die
  Anzeige liegen in der App (der VOP-Manager kennt die aktive Operation).
- **Bilder:** zunaechst **schematische Platzhalter** (Vorher/Nachher-Skizzen,
  Cursor-Symbole, Pfeile — generiert), spaeter durch echte Screenshots
  ersetzbar. Bilder muessen nicht die ganze App zeigen (Ausschnitte).

## Konsistenz ueber alle Plattformen

OpenVED soll auf **macOS, Windows, Linux und WASM** moeglichst **denselben Code
und dasselbe Verhalten** haben (WASM hat nur bewusste Abweichungen: weniger
Fonts, Druck via PDF-Export statt nativ). Die Hilfe folgt diesem Grundsatz:
**QTextBrowser + HTML** laeuft auf allen vier Plattformen identisch. Das ist der
Hauptgrund, warum **QtHelp verworfen** wurde (siehe unten).

### QtHelp (erwogen, verworfen)
Qt Help Framework (`.qhp`→`.qch`, QHelpEngine, Volltextsuche/Index) wurde
geprueft, aber verworfen:
- **WASM:** `.qch` ist eine SQLite-DB; QHelpEngine in WASM kaum unterstuetzt,
  Qt-Assistant laeuft in WASM gar nicht → widerspricht dem Konsistenz-Grundsatz.
- **Web:** `.qch` ist im Browser nicht nutzbar → separate HTML waere trotzdem
  noetig (Doppelpflege).
- **Plugins:** monolithische `.qch` erschwert das Beisteuern durch Plugins.
- Vorteil (Suche/Index) ist bei Bedarf im HTML-Ansatz **nachruestbar**
  (clientseitig).

## Plugin-Erweiterbarkeit

In einer parallelen Story entsteht ein **Plugin-System**. Jedes Plugin soll
**eigene Hilfe** mitbringen koennen. Die Hilfe-Architektur beruecksichtigt das:
- **Hilfe-Registry** (in der App): Eintraege `{ helpId, Titel, Kategorie,
  HTML-Quelle }`. Der Core registriert seine Operationen; **Plugins registrieren
  ihre eigenen Eintraege** (helpId + HTML + TOC-Platzierung).
- **Dynamisches TOC:** Das Inhaltsverzeichnis wird zur Laufzeit aus der Registry
  erzeugt (nicht statisch), sodass Plugin-Hilfe automatisch erscheint.
- **HTML-Quelle je Eintrag:** Core aus Qt-Ressource (`:/ved/help/…`), Plugin aus
  seiner eigenen Ressource/seinem Verzeichnis — QTextBrowser laedt beides.
- **F1** funktioniert auch fuer Plugin-Operationen (aktive Operation → helpId →
  Registry → HTML), plattformkonsistent.
- Bezug: `story_*_plugin_system` (parallel in Arbeit).

## Content-Schema pro Operation (VOC/VOM)

```
<h1> Werkzeugname </h1>
Ziel der Operation
Vorbedingung   (z. B. „ein Objekt muss selektiert sein")
Schritte       (nummeriert, inkl. Mauszeiger-Aenderungen)   [Bild je Schritt]
Ergebnis       (Beschreibung)                                 [Bild]
Verwandte Werkzeuge (Links)
```
Nicht-Operations-Eintraege (Datei/Bearbeiten/Ansicht) analog, ohne Vorbedingung/
Schritte-Zwang.

## Zu dokumentierende Werkzeuge

**Erstellen (VOC):** line · circle (diagonal/diameter/edge/midpoint) ·
ellipse (midpoint/orthogonal) · rectangle (rotated/notrotated) · roundrectangle ·
polygon (smartline) · polycurve · beziercurve · bspline · vectext · vecframetext.

**Bearbeiten (VOM):** select (object / move-node-object / scale-frame) ·
move (object/node/bspline-controlpoint) · rotate (activepoint) · scale (3point) ·
node (insert/delete/change-edge-round) · delete-object · insert-objects ·
modify-curve-attribute · vectext.

**Menue-Aktionen (nicht-Operation):** Datei (Neu/Oeffnen/Speichern/PDF-Export/
Druck) · Bearbeiten · Ansicht (Zoom/Pan) · Fonts · Hilfe.

---

## Step 1: Geruest — Hilfe-Fenster, HTML-Struktur, F1-Infrastruktur

### Was
- **Hilfe-Menue** in `MainWindow` mit Eintrag „OpenVED-Hilfe" (oeffnet TOC) und
  „Hilfe zum aktuellen Werkzeug (F1)".
- **Hilfe-Fenster** (`HelpWindow`, `QTextBrowser` in `QDialog`/`QMainWindow`):
  laedt HTML aus `:/ved/help/…`, folgt internen Links (Navigation), Zurueck/
  Vor-Buttons. WASM-tauglich.
- **Hilfe-Registry** (`HelpRegistry` in der App): erweiterbare Liste von
  Eintraegen `{ helpId, Titel, Kategorie, HTML-Quelle }`; der Core registriert
  seine Eintraege, **Plugins koennen spaeter beisteuern**. Das **TOC wird daraus
  dynamisch erzeugt** (nicht statisch), damit Plugin-Hilfe automatisch erscheint.
- **HTML-Grundstruktur** unter `src/app/resources/help/`:
  Allgemein-Seite „Was ist OpenVED", `style.css`, `ops/` (wird gefuellt),
  `images/`; die TOC-Seite wird zur Laufzeit aus der Registry generiert.
- **Qt-Ressource** (`.qrc`) fuer `help/**` ergaenzen.
- **F1-Infrastruktur:** `helpId()` an der Operation (bzw. Mapping im VOP-Manager
  aktive-Operation → Basisname); globaler `QShortcut`/Event fuer F1 in
  `QVedWidget`/`MainWindow` → oeffnet `HelpWindow` mit `ops/<helpId>.html`
  (Fallback: TOC, wenn keine Operation aktiv/kein Eintrag).

### Tests
- [ ] Hilfe-Menue oeffnet das Fenster mit der Allgemein-Seite (nativ + WASM).
- [ ] F1 ohne aktive Operation oeffnet das TOC.
- [ ] Interne Links im QTextBrowser navigieren korrekt.

### Log
_(nach Umsetzung ausfuellen — Branch `story_17_help_step_1`)_

---

## Step 2: Muster-Operation (voc_line) als Vorlage

### Was
- `help/ops/voc_line.html` komplett nach Content-Schema (Ziel/Vorbedingung/
  Schritte/Ergebnis) mit **schematischen Platzhalter-Bildern**.
- Platzhalter-Bild-Ansatz festlegen (SVG/PNG, Namensschema, Cursor-Symbole,
  Vorher/Nachher) — als wiederverwendbare Vorlage fuer alle weiteren.
- F1 bei aktiver Linien-Operation oeffnet `voc_line.html`.
- `index.html`-TOC verlinkt `voc_line`.

### Tests
- [ ] Linien-Werkzeug aktiv + F1 → `voc_line.html` erscheint.
- [ ] Bilder werden im QTextBrowser (und im Web) angezeigt.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 3: Alle Erstellen-Werkzeuge (VOC)

### Was
- Je eine `help/ops/voc_*.html` fuer alle VOC-Operationen (Liste oben), nach
  Schema, mit Platzhalter-Bildern; TOC + F1-Mapping ergaenzen.

### Tests
- [ ] Jedes VOC-Werkzeug: F1 oeffnet die passende Seite; TOC vollstaendig.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 4: Alle Bearbeiten-Werkzeuge (VOM)

### Was
- Je eine `help/ops/vom_*.html` fuer alle VOM-Operationen, mit besonderem Fokus
  auf **Vorbedingung** (Selektion noetig?) und **Mauszeiger-Aenderungen** je
  Schritt; TOC + F1-Mapping ergaenzen.

### Tests
- [ ] Jedes VOM-Werkzeug: F1 oeffnet die passende Seite; Vorbedingungen korrekt.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 5: Menue-Aktionen (nicht-Operation) + Allgemein-Teil

### Was
- Seiten fuer Datei/Bearbeiten/Ansicht/Fonts + eine ausfuehrliche
  Allgemein-/Einfuehrungsseite (Konzepte: Workspace, Papier-Schablone,
  Real-Einheiten, Koordinatensystem).

### Tests
- [ ] Alle Menue-Eintraege sind dokumentiert und ueber das TOC erreichbar.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 6: Web-Veroeffentlichung (GitHub Pages)

### Was
- Das `help/`-Verzeichnis zusaetzlich auf GitHub Pages unter `/help/`
  veroeffentlichen (im `build-wasm`/Pages-Deploy mitliefern oder eigener Schritt).
- README + In-App-„Online-Hilfe"-Link auf `…github.io/OpenVED/help/`.

### Tests
- [ ] `…github.io/OpenVED/help/` laedt und ist navigierbar.

### Log
_(nach Umsetzung ausfuellen)_

---

## Step 7 (spaeter): Platzhalter-Bilder → echte Screenshots

Die schematischen Platzhalter durch echte Screenshots (Operation-Zwischen-
zustaende, Mauszeiger) ersetzen — gleiche Dateinamen, kein HTML-Umbau noetig.

---

## Akzeptanzkriterien
- Hilfe-Menue + F1-Kontexthilfe funktionieren nativ **und** in WASM.
- Jede Operation hat eine eigene HTML; TOC steckt sie zur Gesamt-Hilfe zusammen.
- Content-Schema (Ziel/Vorbedingung/Schritte/Ergebnis + Bild) je Operation.
- Dieselben Seiten sind im Web unter `/help/` erreichbar.
- `ved_core` bleibt Qt-frei (Hilfe-Anzeige nur in der App).

## Reihenfolge
1. Step 1 — Geruest (Fenster, HTML-Struktur, F1-Infrastruktur).
2. Step 2 — Muster-Operation `voc_line`.
3. Step 3 — VOC-Werkzeuge.
4. Step 4 — VOM-Werkzeuge.
5. Step 5 — Menue-Aktionen + Allgemein.
6. Step 6 — Web-Veroeffentlichung.
7. Step 7 — echte Screenshots (spaeter).

## Bezug
- `plan_installer_build_github_distribution.md` — Pages-Deploy (Step 6 nutzt den
  bestehenden `deploy-pages`-Job).
- `story_16_webassembly.md` — QTextBrowser-Hilfe muss WASM-tauglich sein.
