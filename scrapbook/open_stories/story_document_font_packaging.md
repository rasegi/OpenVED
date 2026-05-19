# Story: Dokument-Fonts persistieren und mitliefern

Datum: 2026-05-19
Status: offen

## Kontext

Textobjekte in VED speichern aktuell Font-Referenzen wie `TT:Segoe UI` oder
`VC:WPS Default`, aber nicht zwingend die Fontdaten selbst. Beim Laden wird der
Fontname ueber den aktuellen `TDFontManager` aufgeloest. Wenn der Font auf dem
Zielsystem fehlt, greift ein Fallback:

1. angeforderter Font
2. `TT:Arial Unicode MS`
3. `TT:Segoe UI`
4. `VC:WPS Default`
5. sichtbarer Missing-Glyph-Marker bei fehlendem Zeichen

Das verhindert unsichtbaren Text, ist aber keine echte Dokumenttreue. Ein
Dokument mit persischem, arabischem, CJK- oder sonstigem Unicode-Text kann auf
einem anderen Rechner anders aussehen oder nur Marker zeigen, wenn die passende
Schrift nicht installiert ist.

Die ordentliche Loesung ist, die fuer das Dokument benoetigten Fonts mit dem
Dokument zu persistieren:

- als Sidecar neben der `.ved`-Datei
- oder direkt eingebettet in die `.ved`-Datei
- optional spaeter als Glyph-/Font-Subset statt kompletter Fontdatei

## Ziel

VED-Dokumente sollen auch auf Rechnern ohne lokal installierte Originalfonts
moeglichst dokumenttreu ladbar und darstellbar sein.

Beim Speichern werden die relevanten Dokument-Fonts erkannt und mitgeliefert.
Beim Laden werden mitgelieferte Dokument-Fonts vor Systemfonts verwendet.

Gewuenschte Aufloesungsreihenfolge:

1. im Dokument eingebetteter Font
2. Sidecar-Font neben dem Dokument
3. installierter Systemfont mit passender Font-ID
4. bekannte System-Fallbacks (`Arial Unicode MS`, `Segoe UI`)
5. `wps_default`
6. Missing-Glyph-Marker fuer einzelne nicht vorhandene Glyphen

Der aktuelle Marker-Fallback bleibt weiterhin noetig, weil auch ein korrekt
geladener Font einzelne Zeichen nicht abdecken kann.

## Nicht-Ziele

- Kein sofortiger vollstaendiger Font-Subsetter.
- Kein Font-Editor.
- Keine automatische Online-Font-Suche.
- Keine Lizenzumgehung fuer nicht einbettbare Fonts.
- Kein Entfernen des aktuellen Font-Fallbacks.
- Keine Qt-Abhaengigkeit im Core-Dateiformat.

## Anforderungen

### Dokumenttreue

- Textobjekte behalten ihren originalen Textinhalt.
- Textobjekte koennen auf eingebettete oder sidecar Fonts verweisen.
- Beim Laden sollen Dokument-Fonts Vorrang vor gleichnamigen Systemfonts haben.
- Wenn ein Dokument-Font fehlt oder nicht lesbar ist, muss das Dokument trotzdem
  ueber Fallbacks ladbar bleiben.

### Font-Paketierung

VED soll mindestens eine der beiden Varianten unterstuetzen:

- **Sidecar-Verzeichnis**
  - Beispiel: `zeichnung.ved_fonts/`
  - enthaelt `.vfn`, `.ttf`, `.otf` oder spaeter konvertierte/cachebare Fonts
  - Vorteil: einfach zu debuggen und grosse Fonts bleiben ausserhalb der
    Hauptdatei

- **Eingebetteter Font-Chunk**
  - Fontdaten liegen direkt in der `.ved`
  - Vorteil: eine einzelne Datei ist portabel
  - Nachteil: groessere Dateien und mehr Formatlogik

Langfristig kann beides existieren:

- embedded fuer "eine Datei weitergeben"
- sidecar fuer Arbeitsverzeichnisse und grosse Projekte

### Font-Metadaten

Pro Dokument-Font sollten stabile Metadaten gespeichert werden:

- dokumentinterne Font-ID
- urspruengliche Font-ID, z.B. `TT:Segoe UI`
- Anzeigename
- Font-Typ (`VFN`, `TTF`, `OTF`, spaeter `SUBSET`)
- Style-Informationen, soweit verfuegbar
- Hash der Fontdaten
- Lizenz-/Embedding-Status, wenn ermittelt werden kann

### Lizenz und Einbettbarkeit

TrueType/OpenType-Fonts koennen Einbettungsrechte enthalten. Vor dem
automatischen Einbetten muss geprueft werden, ob der Font technisch und
rechtlich eingebettet werden darf.

MVP-Regel:

- VFN-Fonts duerfen eingebettet werden.
- TTF/OTF-Fonts werden nur eingebettet, wenn die Embedding-Rechte das erlauben
  oder der Benutzer dies explizit bestaetigt.
- Wenn ein Font nicht eingebettet wird, speichert VED trotzdem die Font-ID und
  zeigt beim Speichern eine Warnung.

## Architekturvorschlag

### 1. Document-Font-Provider

Der bestehende `TDFontManager` sollte einen dokumentbezogenen Provider bekommen:

```text
DocumentEmbeddedFontProvider
DocumentSidecarFontProvider
SystemFontProvider
BuiltinVfnFontProvider
```

Der `TDFontManager` muss Dokument-Fonts vor globalen Systemfonts aufloesen.

### 2. Dateiformat erweitern

Das VED-Dateiformat bekommt einen optionalen Font-Chunk.

Moeglicher Chunk:

```text
FONT
  font-count
  font-record*

font-record
  document-font-id
  original-font-id
  display-name
  font-kind
  payload-kind
  payload-size
  payload-bytes
  hash
```

Alternativ kann der `FONT`-Chunk nur Metadaten enthalten und auf Sidecar-Dateien
verweisen.

### 3. Save-Pipeline

Beim Speichern:

1. Alle `TDVecText`/`TDVecFrameText` im Model sammeln.
2. Verwendete Font-IDs bestimmen.
3. Fonts aus dem `TDFontManager` holen.
4. Pro Font entscheiden:
   - einbettbar
   - sidecar speichern
   - nur referenzieren und warnen
5. Font-Metadaten und ggf. Fontdaten schreiben.
6. Textobjekte behalten ihre Font-ID; optional wird eine dokumentinterne
   Font-ID ergaenzt.

### 4. Load-Pipeline

Beim Laden:

1. Font-Chunks oder Sidecar-Fonts zuerst lesen.
2. Dokument-Font-Provider fuer diese Fonts erzeugen.
3. `TDFontManager` mit Dokument-Provider registrieren.
4. Textobjekte ueber die normale Font-Resolution aufloesen.
5. Warnungen erzeugen, wenn Dokument-Fonts fehlen, defekt oder nicht lesbar sind.

### 5. UI-Verhalten

Beim Speichern:

- Warnung, wenn verwendete Systemfonts nicht eingebettet werden koennen.
- Option "Fonts mit Dokument speichern", sobald die technische Basis existiert.
- Spaeter Option "eine portable Datei erzeugen".

Beim Laden:

- Warnung, wenn ein Font ersetzt werden musste.
- In der Font-Auswahl sollte sichtbar sein, ob ein Font aus dem Dokument, aus dem
  Sidecar, vom System oder aus dem Built-in-Fallback kommt.

## Akzeptanzkriterien MVP

- Ein Dokument mit einem mitgelieferten VFN-Font laedt auf einem System ohne
  diesen Font korrekt.
- Dokument-Fonts werden vor Systemfonts verwendet.
- Wenn ein Dokument-Font fehlt, greift weiterhin die bestehende
  Fallback-Reihenfolge.
- Ein Text mit fehlenden Einzelglyphen zeigt weiterhin Marker statt unsichtbar
  zu verschwinden.
- Tests decken ab:
  - eingebetteter Dokument-Font wird bevorzugt
  - Sidecar-Font wird bevorzugt
  - fehlender Dokument-Font faellt auf `Segoe UI`/`wps_default` zurueck
  - Originaltext bleibt unveraendert

## Offene Entscheidungen

- Erst Sidecar oder erst eingebetteter `FONT`-Chunk?
- Ganze Fontdatei oder direkt konvertierter `TDVecFont`?
- Wann und wie wird ein TTF/OTF-Subset erzeugt?
- Wie detailliert sollen Embedding-Rechte im MVP geprueft werden?
- Soll die portable Datei ein einzelnes `.ved` bleiben oder optional ein Paket
  sein?

## Bezug zu bestehenden Stories

- `story_ved_document_persistence_v1_0_1.md`
  - erweitert die Dokument-Persistence um Font-Paketierung

- `story_font_provider_truetype_converter_plugin.md`
  - liefert die Grundlage fuer Systemfont-Aufloesung und TrueType-Konvertierung

- aktueller Glyph-Fallback
  - bleibt als letzte Sicherheitsstufe bestehen und ersetzt keine
    Dokument-Font-Persistenz
