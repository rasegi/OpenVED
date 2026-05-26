# Story: Arbeitsvorlagen und Neues-Dokument-Dialog

Datum: 2026-05-26
Status: offen

## Kontext

Das Konzept `concept_units_pages_templates_pdf_print.md` definiert in Step 7
Arbeitsvorlagen (Document Templates), die beim Erstellen eines neuen Dokuments
zur Auswahl stehen sollen.

Voraussetzung ist, dass die Core-Typen aus
`story_core_units_grid_rulers_page_settings.md` bereits existieren:

- `TDVecDocumentSettings`
- `TDVecUnitSettings`
- `TDVecGridSettings`
- `TDVecPageSettings`
- `TDVecPageFormats`

Aktuell entsteht ein neues Dokument mit fest kodierten Defaults
(`210000 x 296985`, Grid `10000/10`). Es gibt keinen Dialog und keine
Vorlagenauswahl.

## Ziel

Beim Erstellen eines neuen Dokuments soll der Benutzer aus vordefinierten
Arbeitsvorlagen waehlen koennen. Jede Vorlage erzeugt ein vollstaendiges
`TDVecDocumentSettings`.

## Nicht-Ziele

- Kein benutzerdefinierter Vorlagen-Editor (eigene Vorlagen erstellen/speichern).
- Keine Multi-Page- oder Artboard-Auswahl.
- Kein PDF/Print — das ist eine eigene Story.

## Vorgeschlagene Core-Architektur

### Document Template

```cpp
struct TDVecDocumentTemplate {
    std::string id;
    std::string displayName;
    TDVecDocumentSettings documentSettings;
};
```

### Template Registry

```cpp
class TDVecDocumentTemplateRegistry {
public:
    TDVecDocumentTemplateRegistry();

    const std::vector<TDVecDocumentTemplate>& Templates() const;
    const TDVecDocumentTemplate& DefaultTemplate() const;
    const TDVecDocumentTemplate* FindById(const std::string& id) const;

private:
    std::vector<TDVecDocumentTemplate> templates_;
};
```

### Vordefinierte Vorlagen

| ID | Name | Einheit | Seite | Orientierung |
|---|---|---|---|---|
| `a4_mm_portrait` | A4 Portrait (mm) | Millimeter | 210 x 297 mm | Portrait |
| `a4_mm_landscape` | A4 Landscape (mm) | Millimeter | 297 x 210 mm | Landscape |
| `a3_mm_portrait` | A3 Portrait (mm) | Millimeter | 297 x 420 mm | Portrait |
| `a3_mm_landscape` | A3 Landscape (mm) | Millimeter | 420 x 297 mm | Landscape |
| `a5_mm_portrait` | A5 Portrait (mm) | Millimeter | 148 x 210 mm | Portrait |
| `letter_in_portrait` | Letter Portrait (in) | Inch | 8.5 x 11 in | Portrait |
| `custom` | Custom | Millimeter | benutzerdefiniert | benutzerdefiniert |

Default-Vorlage: `a4_mm_portrait`.

Grid-Defaults leiten sich aus der Einheit ab:

- Millimeter: major = 10 mm, subdivisions = 10
- Inch: major = 1 in, subdivisions = 8

## Qt-Integration

### Neues-Dokument-Dialog

Ein modaler Dialog (`QNewDocumentDialog`) zeigt die verfuegbaren Vorlagen.

Mindestumfang:

- Liste oder Grid der Vorlagen
- Vorschau der Seitengroesse und Einheit
- Bei Auswahl von `custom`: Eingabefelder fuer Breite, Hoehe, Einheit
- OK / Abbrechen

### Integration in MainWindow

- Menue `File > New...` oeffnet den Dialog.
- Bei OK wird `TDVecDocumentSettings` aus der gewaehlten Vorlage erzeugt.
- Das Model wird mit diesen Settings initialisiert.
- Der bestehende `File > New`-Shortcut ohne Dialog bleibt erhalten und
  verwendet die Default-Vorlage (`a4_mm_portrait`).

## Vorgeschlagene Umsetzungsschritte

### Step 1: Core Template-Typen

- `TDVecDocumentTemplate`
- `TDVecDocumentTemplateRegistry` mit vordefinierten Vorlagen
- Tests: alle Vorlagen erzeugen gueltige `TDVecDocumentSettings`

### Step 2: Qt Neues-Dokument-Dialog

- `QNewDocumentDialog` mit Vorlagenauswahl
- Custom-Eingabe fuer Breite, Hoehe, Einheit
- Integration in MainWindow

### Step 3: Model-Initialisierung

- `File > New...` erzeugt Model mit gewaehlten Settings
- `File > New` (ohne Dialog) nutzt Default-Vorlage
- Bestehende Defaults werden durch Template-Default ersetzt

## Abhaengigkeiten

- `story_core_units_grid_rulers_page_settings.md`:
  - **Step A** (Core-Datentypen: `TDVecDisplayUnit`, `TDVecUnitSettings`,
    `TDVecUnitFormatter`) — **offen**
  - **Step C** (DocumentSettings im Model: `TDVecDocumentSettings`,
    `TDVecPageSettings`, `TDVecGridSettings`, `TDVecPageFormats`) — **offen**
- `concept_units_pages_templates_pdf_print.md` — Konzept Step 7

## Akzeptanzkriterien

- Vordefinierte Vorlagen sind im Core registriert und per ID abrufbar.
- `File > New...` oeffnet einen Dialog mit Vorlagenauswahl.
- Die gewaehlte Vorlage erzeugt korrekte `TDVecDocumentSettings`.
- Grid, Lineal und Statusbar spiegeln die gewaehlte Einheit wider.
- `File > New` ohne Dialog verwendet die Default-Vorlage.
- Custom-Vorlage erlaubt manuelle Eingabe von Breite, Hoehe und Einheit.
