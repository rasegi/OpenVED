# Story: Code-Style vereinheitlichen & erzwingen (.clang-format + Checks)

Datum: 2026-07-31
Status: offen

## Kontext

Der C++-Stil in OpenVED ist heute **nicht erzwungen**: kein `.clang-format`, kein
`.clang-tidy`, kein `.editorconfig`, keine CI-Pruefung. Das **Layout** ist intern
weitgehend konsistent (4-Space-Indent, Spaces, `PascalCase()`-Methoden), das
**Naming** dagegen **gespalten** entlang der Trennlinie Alt-Code vs. neuerer Code
(vollstaendige Bestandsaufnahme unten unter „Naming-Ist-Analyse"). Verstoesse
gegen die **C++ Core Guidelines** und Inkonsistenzen:

- **Reservierte Namen:** 33 Header nutzen Guards wie `#ifndef __VOC_LINE_H`
  (fuehrendes `__` + Grossbuchstaben → laut `[lex.name]` der Implementierung
  reserviert; Core Guideline **SF.12/NL**).
- **Inkonsistente Header-Guards:** 37× `#pragma once` vs. 36× `#ifndef` (davon 33
  reserviert).
- **Member-Naming gespalten:** ~3789× Hungarian `m`-Praefix (`mpVecModel`,
  `mnResolution`) vs. ~244× trailing `_` (`member_`) — dominant ist **Hungarian**,
  nicht `_` (Korrektur einer frueheren Annahme).
- **Hungarian Pointer-Praefix:** >1300× `p*` (`pGE`, `pVecModel`, `pVecEditCad`)
  (**NL.5**).
- **Uneinheitliche Typ-Praefixe:** `TDVec` / `TDVO*` / `TDMat*` / `TD*` / `VED*` /
  praefixlos (Details unten).
- **camelCase-Core-Funktionen:** ~18 freie Funktionen brechen die PascalCase-Regel.
- **Zeilenlaenge** teils bis Spalte 134 (kein Limit).

## Naming-Ist-Analyse (vollstaendige Bestandsaufnahme, 2026-07-31)

Analyse ueber alle `src/**` + `tests/**` (Fremdtypen `Q*`/`FT_*`/`hb_*` heraus-
gefiltert). Die Inkonsistenzen folgen fast durchgehend **Alt-Code** (Hungarian,
`__`-Guards, `(void)`) vs. **neuerer Code** (trailing `_`, `#pragma once`, `nullptr`).

### Member-Naming — groesster Bruch
| Stil | Vorkommen | Bereich |
|---|---|---|
| Hungarian `m`-Praefix (`mp`=ptr, `mn`=num, `mb`=bool, `me`=enum) | ~3789 (37 Header) | Alt-Code: alle `operations/`, `vec_edit*`, `vec_font.h`, `vec_view_interface.h`, `vec_text.h` |
| trailing `_` (`member_`) | ~244 (27 Header) | neuer Code: `serialization/`, `vecobjects/`, `main/vec_model.h`, `vec_units.h`, `gengine/`, App |

Gemischt in **einer** Datei: `vec_font.h`, `vom_insert_objects.h`, `vop_manager.h`,
`vec_text.h`. Dritte Variante: `m` ohne Typbuchstabe (`mMatLine`, `msText`).
→ Dominant ist **Hungarian** (~94 %), nicht `_`.

### Typ-Praefixe — vier Schemata + praefixlos
| Praefix | Anzahl | Bereich |
|---|---|---|
| `TDVec*` | ~39 | vecobjects, main, fontengine |
| `TDVO*` (VO = View Operation) | ~40 | `operations/` (konsistentes Sub-Schema) |
| `TDMat*` | 6 | mathlib |
| `TD*` (ohne Vec/VO/Mat) | 6 | `TDGraphicEngine`, `TDFontManager`, `TDGraphicEngineQt`, … |
| `VED*` (all-caps) | 9 | `serialization/` (`VEDBinaryReader`, …) |
| `I*` | 1 | `IVecFontProvider` (Interface, korrekt) |
| praefixlos | ~10 | `FrameExtents`, `FreeTypeFace`, `OutlineContext`, `FontEntry`, `TextRun`, `VecShapingCache`, … |

App-Layer bewusst im Qt-Stil (`MainWindow`, `DocumentSetupDialog`, `QVedWidget`).

### Methoden
Klassenmethoden **100 % PascalCase**. Ausnahme: ~18 **freie** camelCase-Funktionen
in `vec_object_geometry.h` (`atanD2`, `distancePointToSegment`, `makeFrameFromBounds`,
…) + `vec_math_base.h` (`computeCoefficients`). Qt-Overrides (`paintEvent`, …) erlaubt.

### Dateinamen
Core **100 % `snake_case`**; App `PascalCase` (Ausnahme: `main.cpp`); Tests
`snake_case`. Datei↔Klasse-Mapping ueberwiegend sauber; einziger Casing-Bruch:
`TDVocLineExtVar` vs. `TDVOCLine` in `voc_line.h`.

### Weitere Hygiene
- **konform:** kein `using namespace` in Headern; saubere Include-Reihenfolge; alle
  Header guard-geschuetzt; 319:5 `nullptr`:`NULL`; 488 `static_cast` (kaum C-Casts).
- **offen:** 1 nicht-const globale `gVecTextShaper` (`vec_text.cpp:21`, I.2); 5× `NULL`
  (`vop_base.*`, `vec_view_interface.cpp`); 342× `(void)`-Parameterlisten (C-ism).

## Wichtige Klarstellung (Scope-Grenze)

`.clang-format` erzwingt nur **Layout/Whitespace** (Einrueckung, Klammern,
Zeilenumbrueche, Include-Sortierung, Zeilenlaenge). Es aendert **kein Naming**.
Die naming-bezogenen Guideline-Punkte werden getrennt behandelt:

| Punkt | Werkzeug |
|---|---|
| Einrueckung, Klammern, Zeilenlaenge, Include-Order | **`.clang-format`** (Step 1) |
| Reservierte `__`-Header-Guards | **manuell/`#pragma once`** (Step 2) — clang-format kann das nicht |
| Naming-Konventionen (Hungarian, PascalCase-Policy) | **`.clang-tidy`** `readability-identifier-naming` (Step 5, optional) |

## Ziel

- Ein `.clang-format`, das einen **anerkannten Standard-Basisstil** durchsetzt und
  im ganzen Repo gilt.
- Code **einmalig** repo-weit reformatiert (eigener, isolierter Commit).
- Format-**Check im Build/CI**, der bei Abweichung fehlschlaegt.
- Format-Integration in **CLion** (nutzt `.clang-format` automatisch).
- Die reservierten Header-Guards sind behoben.

## Nicht-Ziele

- **Kein** Umbau der Naming-Konventionen (`TDVec`-Praefix, `PascalCase`-Methoden
  bleiben) — nur optional als spaeterer clang-tidy-Schritt (Step 5).
- **Keine** funktionalen/logischen Aenderungen — reine Formatierung/Guards.
- Kein Aufbrechen bestehender Reviews: der grosse Reformat-Commit bleibt **separat**
  und wird von Logik-Commits getrennt gehalten.

## Zentrale Entscheidung: Basisstil (praegt Diff-Umfang)

`.clang-format` baut auf einem `BasedOnStyle` auf. Wahl bestimmt den Reformat-Diff:

| Basisstil | Indent | Charakter | Diff ggü. Ist (4-Space) |
|---|---|---|---|
| **LLVM** | 2 | C++-Ökosystem-Default, Core-Guidelines-nah | **gross** (4→2) |
| **Google** | 2 | streng, verbreitet | **gross** (4→2) |
| **Microsoft** | 4 | 4-Space, Allman-Braces | mittel |
| **LLVM + Overrides** (`IndentWidth: 4`, `ColumnLimit: 100`) | 4 | Standard-Basis, aber Ist-nah | **klein** |

**ENTSCHIEDEN (2026-07-31): reines LLVM, nur Spaltenlimit angehoben.** Prioritaet ist
**maximale Standard-Naehe**, Diff-Umfang bewusst egal. `.clang-format` im Repo-Root:
`BasedOnStyle: LLVM`, `Standard: Latest`, `ColumnLimit: 120` — sonst **LLVM-Defaults**
(u.a. **IndentWidth 2**, **PointerAlignment Right** → `int *p`, Attach-Braces).

**Reformat-Umfang gemessen** (clang-format 19.1.5): **178 / 178 Dateien**, ~52.000
geaenderte Zeilen — praktisch das gesamte Repo, weil die 2-Space-Einrueckung jede
eingerueckte Zeile beruehrt. Bewusst akzeptiert zugunsten voller Standard-Naehe.
(Zum Vergleich: eine Ist-nahe 4-Space-Variante waere ~8.900 Zeilen / 153 Dateien —
verworfen, weil weniger standardnah.) Der Reformat wird ein einmaliger, isolierter
Commit (Step 1, `.git-blame-ignore-revs`).

## Step 1: `.clang-format` + einmaliger Repo-Reformat

### Was
- `.clang-format` im Repo-Root anlegen (Basisstil laut Entscheidung oben).
- `clang-format -i` ueber alle `src/**` und `tests/**` `.h/.cpp` laufen lassen.
- Als **eigenen** Commit "style: repo-wide clang-format" isolieren (leichter zu
  reviewen/blamen; `.git-blame-ignore-revs` eintragen, damit `git blame` den
  Reformat-Commit ueberspringt).

### Tests
- [ ] `clang-format --dry-run --Werror` ueber alle Dateien meldet **null**
      Abweichungen nach dem Reformat.
- [ ] Projekt baut nativ + WASM unveraendert; alle Tests gruen (reine Formatierung).

### Log
_(nach Umsetzung ausfuellen)_

## Step 2: Header-Guards vereinheitlichen (reservierte Namen beheben)

### Was
- Alle 33 `#ifndef __XXX_H`-Guards + die restlichen `#ifndef`-Guards auf **`#pragma
  once`** umstellen (behebt zugleich die reservierten `__`-Namen und die 50/50-
  Inkonsistenz). `#pragma once` wird von allen Ziel-Compilern (MSVC, Clang/Emscripten,
  GCC) unterstuetzt.
- Alternativ (falls `#pragma once` unerwuenscht): Guards auf nicht-reservierte Namen
  umbenennen (`VED_..._H` ohne fuehrendes `__`).

### Tests
- [ ] Keine `#ifndef __`-Guards mehr im Repo (`grep` leer).
- [ ] Kein doppelt-inkludierter Header / Build gruen.

### Log
_(nach Umsetzung ausfuellen)_

## Step 3: Format-Check im Build/CI

### Was
- **CMake-Target** `format-check`: laeuft `clang-format --dry-run --Werror` ueber die
  Quell-Dateiliste; schlaegt bei Abweichung fehl. Optional `format-fix` (`-i`).
- **CI-Job** (GitHub Actions, neuer Workflow `format.yml` oder Job in bestehendem):
  auf ubuntu `clang-format` installieren, `format-check` ausfuehren; PR/Push rot bei
  Verstoss. (Ergaenzt `release.yml`.)
- Optional: **pre-commit-Hook** (`.githooks/pre-commit`), der geaenderte Dateien
  prueft — als lokale Absicherung vor dem CI.

### Tests
- [ ] `cmake --build . --target format-check` schlaegt bei absichtlich verunstalteter
      Datei fehl und ist gruen nach clang-format.
- [ ] CI-Job blockt einen PR mit Formatverstoss.

### Log
_(nach Umsetzung ausfuellen)_

## Step 4: CLion-Integration

### Was
- CLion erkennt ein `.clang-format` im Projekt-Root **automatisch** und nutzt es
  fuer "Reformat Code" (Settings → Editor → Code Style: "Enable ClangFormat" ist bei
  vorhandener Datei default). In der Story dokumentieren, wie man es verifiziert.
- **Reformat/Optimize on save** aktivieren (Settings → Tools → Actions on Save →
  "Reformat code"), damit gespeicherte Dateien konform bleiben.
- Kurzer Abschnitt in `README`/`CONTRIBUTING` mit dem CLion-Setup (2-3 Schritte).

### Tests
- [ ] In CLion formatiert "Reformat Code" gemaess `.clang-format` (nicht dem
      IDE-Default).
- [ ] Actions-on-Save haelt eine Datei nach dem Speichern konform.

### Log
_(nach Umsetzung ausfuellen)_

## Step 5 (optional, gross): Naming vereinheitlichen (clang-tidy + manuell)

Basierend auf der **Naming-Ist-Analyse** oben. Bewusst optional und **getrennt** von
Layout/Guards (grosse, rein kosmetische Diffs). Je Aspekt zuerst eine **Ziel-
Konvention entscheiden** (siehe „Offene Entscheidungen").

### Aspekte (nach Diff-Groesse)
- **Member-Naming (groesster Diff) — entschieden: trailing `_`.** Die ~3789×
  Hungarian `m*` (`mpVecModel`, `mnResolution`, `mb…`, `me…`) auf `member_`
  vereinheitlichen; die 4 gemischten Dateien (`vec_font.h`, `vom_insert_objects.h`,
  `vop_manager.h`, `vec_text.h`) zuerst. Betrifft fast jede Klasse.
- **Hungarian Pointer-`p*`** (>1300×, NL.5) abbauen.
- **Typ-Praefixe — entschieden: Familien bleiben, nur praefixlose angleichen.**
  `TDVec*`/`TDVO*`/`TDMat*`/`VED*`/`TD*` und `I*` (Interfaces) bleiben; nur die
  **praefixlosen** Core-nahen Typen (`FrameExtents`, `OutlineContext`, `FontEntry`,
  `TextRun`, `VecShapingCache`, `FreeTypeFace`, …) bekommen ein passendes Praefix
  (Datei↔Klasse-Mapping halten).
- **camelCase-Core-Funktionen** (`vec_object_geometry.h`, `vec_math_base.h`) →
  PascalCase.
- **Kleinfaelle:** `TDVocLineExtVar`→`TDVOCLineExtVar`; nicht-const globale
  `gVecTextShaper` entkoppeln (I.2); 5× `NULL`→`nullptr`; `main.cpp`-Casing (falls
  App-PascalCase erzwungen wird).

### Werkzeuge
- `.clang-tidy` mit `readability-identifier-naming` (kodifiziert das **gewaehlte**
  Schema) + ausgewaehlte `cppcoreguidelines-*`/`bugprone-*`-Checks.
- Grosse Umbenennungen per clang-tidy `--fix` / `clang-rename`, in **getrennten,
  aspekt-weisen** Commits (`.git-blame-ignore-revs`).

### Tests
- [ ] `clang-tidy` laeuft ueber die Compile-DB ohne die konfigurierten Checks zu
      verletzen (nach Anpassung je Aspekt).
- [ ] Nativer + WASM-Build gruen, alle Tests gruen (reine Umbenennung).

### Log
_(nach Umsetzung ausfuellen)_

## Entscheidungen (alle getroffen 2026-07-31)

1. **Basisstil** — reines LLVM (2-Space, PointerAlignment Right, Attach-Braces) +
   `ColumnLimit: 120` (siehe „Zentrale Entscheidung").
2. **Header-Guards** — **`#pragma once`** ueberall (behebt die reservierten
   `__`-Namen **und** die 50/50-Inkonsistenz in einem Schritt).
3. **Umfang jetzt** — nur **Steps 1–4** (clang-format + Guards + CI-Check + CLion);
   **Step 5 (Naming) spaeter**, als eigener, getrennter Durchgang.
4. **Member-Konvention (Step 5)** — auf **trailing `_`** vereinheitlichen
   (guideline-naeher). Grosser Diff (fast jede Klasse) → bewusst spaeter/getrennt.
5. **Typ-Praefix-Ziel (Step 5)** — die gewachsenen Familien
   (`TDVec*`/`TDVO*`/`TDMat*`/`VED*`, `I*` fuer Interfaces) **akzeptieren**; nur die
   **praefixlosen** Typen (`FrameExtents`, `OutlineContext`, `FontEntry`, `TextRun`,
   `VecShapingCache`, `FreeTypeFace`, …) an ein passendes Schema angleichen. `TD*`
   (gengine/fontengine) bleibt ebenfalls (keine Zwangs-Umbenennung nach `TDVec`).

## Akzeptanzkriterien

- `.clang-format` im Root; Repo einmalig konform reformatiert.
- `clang-format --dry-run --Werror` ist repo-weit gruen.
- Keine reservierten `__`-Header-Guards mehr.
- Build/CI schlaegt bei Formatverstoss fehl.
- CLion nutzt dieselbe `.clang-format` (kein IDE-Divergenz).
- Nativer + WASM-Build gruen, alle Tests gruen (keine funktionale Aenderung).

## Bezug

- **`concept_plugin_architecture.md`** — unabhaengig, aber ein sauberer, erzwungener
  Stil erleichtert die kommende Plugin-Refaktorisierung (viele Datei-Verschiebungen).
- **`story_16_webassembly.md`** — der CI-Format-Job laeuft neben dem WASM/Release-Job;
  clang-format muss auch die `EMSCRIPTEN`-gegateten Bloecke sauber lassen.
