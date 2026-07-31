# Story: Code-Style vereinheitlichen & erzwingen (.clang-format + Checks)

Datum: 2026-07-31
Status: offen

## Kontext

Der C++-Stil in OpenVED ist heute **nicht erzwungen**: kein `.clang-format`, kein
`.clang-tidy`, kein `.editorconfig`, keine CI-Pruefung. Der Ist-Stil ist intern
weitgehend konsistent (4-Space-Indent, Spaces, `TDVec`-Praefix, `PascalCase()`-
Methoden, `trailing_underscore_`-Member), hat aber Abweichungen und einige
Verstoesse gegen die **C++ Core Guidelines**:

- **Reservierte Namen:** 33 Header nutzen Guards wie `#ifndef __VOC_LINE_H`
  (fuehrendes `__` + Grossbuchstaben → laut `[lex.name]` der Implementierung
  reserviert; Core Guideline **SF.12/NL**).
- **Inkonsistente Header-Guards:** 37× `#pragma once` vs. 36× `#ifndef`.
- **Hungarian Notation:** Pointer-Parameter `pVecModel`, `pVecEditCad` (**NL.5**).
- **Zeilenlaenge** teils bis Spalte 134 (kein Limit).

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

## Step 5 (optional): clang-tidy fuer Naming/Guideline-Checks

### Was
- `.clang-tidy` mit `readability-identifier-naming` (kodifiziert `TDVec`-Praefix,
  Member-`_`-Suffix etc.) und ausgewaehlten `cppcoreguidelines-*`/`bugprone-*`-Checks.
- Hungarian `p`-Praefixe (NL.5) abbauen — **grosser, rein kosmetischer Diff**, daher
  bewusst optional und getrennt.

### Tests
- [ ] `clang-tidy` laeuft ueber die Compile-DB ohne die konfigurierten Checks zu
      verletzen (nach Anpassung).

### Log
_(nach Umsetzung ausfuellen)_

## Offene Entscheidungen

1. ~~**Basisstil**~~ — **entschieden:** reines LLVM (2-Space, PointerAlignment Right,
   Attach-Braces) + ColumnLimit 120 (siehe Abschnitt "Zentrale Entscheidung").
2. **Header-Guards:** `#pragma once` (Empfehlung) vs. nicht-reservierte `#ifndef`.
3. **Umfang jetzt:** Steps 1-4 (Layout + Guards + Checks) jetzt; Step 5 (Naming via
   clang-tidy) spaeter?

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
