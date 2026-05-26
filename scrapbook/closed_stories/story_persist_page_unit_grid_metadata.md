# Story: DocumentSettings im Dateiformat persistieren

Datum: 2026-05-26
Status: umgesetzt am 2026-05-26

## Kontext

Mit der Story `story_core_units_grid_rulers_page_settings` wurden DocumentSettings
eingefuehrt: UnitSettings, GridSettings und PageSettings (inkl. PageOrigin). Diese
Einstellungen leben im `TDVecModel` und werden von der UI genutzt (Page Bounds,
Statusbar, Page-Setup-Dialog).

Das bestehende Dateiformat 1.0.2 persistiert diese Einstellungen **nicht**. Beim
Speichern und erneuten Oeffnen gehen Papierformat, Einheit, Grid-Konfiguration und
Page-Position verloren — es werden immer die Defaults (A4, mm, 10000/10) verwendet.

## Ziel

DocumentSettings werden als neuer `DSET`-Chunk im VED-Dateiformat gespeichert und
beim Laden wiederhergestellt. Alte Dateien ohne `DSET`-Chunk laden weiterhin mit
Defaults — volle Abwaertskompatibilitaet.

## Was wird gemacht

### Dateiformat

Neuer optionaler Chunk `DSET` (Document Settings) im VED-Format:

```text
DSET chunk payload:
  UnitSettings:
    realUnitsPerMillimeter  (double)
    displayUnit             (uint8 — 0=RawVed, 1=mm, 2=cm, 3=in)
  GridSettings:
    majorStepReal           (double)
    subdivisions            (int32)
    resolutionLimitPixels   (int32)
  PageSettings:
    formatName              (string — length-prefixed UTF-8)
    widthReal               (double)
    heightReal              (double)
    pageOriginX             (double)
    pageOriginY             (double)
    orientation             (uint8 — 0=Portrait, 1=Landscape)
```

### Aenderungen

**`ved_model_io.cpp` (Core):**
- `SaveVecModelToBytes`: Neuen `DSET`-Chunk nach `META` schreiben
- `LoadVecModelFromBytes`: `DSET`-Chunk lesen, bei Fehlen Defaults verwenden
- Chunk-Reihenfolge: `META`, `DSET` (optional), `VIEW`, `OBJS`

**`ved_model_io.h` (Core):**
- Keine Signatur-Aenderungen noetig — DocumentSettings sind bereits im Model

**Keine Formatversions-Aenderung** — der Chunk-Mechanismus erlaubt unbekannte Chunks
zu ueberspringen. Aeltere Leser ignorieren `DSET` automatisch.

### Tests (`ved_core_model_io_tests.cpp`)

- [x] Roundtrip: DocumentSettings mit nicht-default Werten speichern/laden
- [x] Roundtrip: PageOrigin != 0 bleibt erhalten
- [x] Roundtrip: Custom-Format mit beliebiger Groesse bleibt erhalten
- [x] Abwaertskompatibilitaet: Datei ohne DSET-Chunk laedt mit Defaults
- [x] Roundtrip mit Objekten: DocumentSettings + Objekte gemeinsam korrekt

## Umsetzungsnotiz 2026-05-26

- `DSET`-Chunk in `ved_model_io.cpp` implementiert (writeDocumentSettings / readDocumentSettings)
- Chunk wird zwischen META und VIEW geschrieben
- Fehlender DSET-Chunk beim Laden → Defaults (A4, mm, 10000/10)
- 5 neue Tests in `ved_core_model_io_tests.cpp`, alle bestanden
- Gesamte Test-Suite: 24/24 gruen, keine Formatversions-Aenderung noetig

## Definition of Done

- [x] DocumentSettings ueberleben einen Save/Open-Roundtrip.
- [x] Alte Dateien ohne DSET laden weiterhin fehlerfrei mit Defaults.
- [x] Bestehende Tests bleiben gruen.
- [x] Neue Roundtrip-Tests fuer DocumentSettings bestehen.
