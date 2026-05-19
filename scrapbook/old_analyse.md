### Bestandsaufnahme: Legacy 2D-CAD Library (borland_ved)

#### 1. Überblick
Der vorliegende Code (ca. 1999/2000) stellt einen soliden 2D-Vektoreditor dar. Er ist modular aufgebaut und trennt weitgehend zwischen Datenmodell, Logik und Darstellung. Die Architektur orientiert sich an MicroStation, was sich insbesondere im cursor-gesteuerten Bedienkonzept widerspiegelt.

#### 2. Modulanalyse
| Modul | Beschreibung | Portierbarkeit auf Qt |
| :--- | :--- | :--- |
| **mathlib** | Geometrische Grundtypen (`TDMatPoint`, `TDMatLine`, `TDMatRect`) und komplexe Berechnungen (Splines, Bezier). | **Hoch.** Reines C++, fast keine Abhängigkeiten. |
| **vecobjects** | Definition der Grafikobjekte (`VECOBJ_LIN`, `VECOBJ_ELL`, etc.). Nutzt ein eigenes Interface-System. | **Mittel.** Das Streaming-System (`TDStreamable`) müsste auf `QDataStream` oder ähnliches umgestellt werden. |
| **main** | Kern-Logik: `TDVecModel` (Datenhaltung) und `TDVecEditCad` (Editor-Status). | **Hoch.** Kernlogik ist weitgehend UI-unabhängig. |
| **operations** | Command-Pattern für Operationen (`VOC_...` zum Erstellen, `VOM_...` zum Ändern). | **Hoch.** Die Logik der Maus-Interaktion ist in `OPMouseDown` etc. abstrahiert. |
| **gengine** | Abstraktionsschicht für Grafik. | **Niedrig.** `TDGraphicEngine` ist zwar abstrakt, die Screen-Implementierung nutzt aber massiv Windows GDI (`HDC`, `windows.h`, `HIMAGELIST`). |
| **vcl** | Borland VCL Komponenten (`TSCVecView`). | **Niedrig.** Muss durch Qt-Widgets (`QWidget` oder `QGraphicsView`) ersetzt werden. |

#### 3. Ressourcen (Icons & Cursor)
- **Cursor (.cur):** Umfangreiche Sammlung spezialisierter Cursor (z.B. `create_circle_1.cur`, `zoom.cur`). In Qt können diese über `QCursor(QPixmap)` geladen werden.
- **Icons (.bmp):** Bitmaps für Knotenpunkte (Nodes) und Toolbars.

#### 4. Kritische Abhängigkeiten & Herausforderungen
- **Windows API:** In `gengine/vec_graphic_engine_screen.h` wird direkt mit `windows.h`, `HDC`, `HPEN`, `HBRUSH` gearbeitet. Dies ist der Teil, der am stärksten überarbeitet werden muss (Ersatz durch `QPainter`).
- **Streaming/Persistence:** Die Klassen erben oft von `TDStreamable`. Die Persistenzschicht muss für Qt modernisiert werden.
- **VCL Message Handling:** Die Integration der Maus- und Tastaturereignisse erfolgt über Borland-spezifische `MESSAGE_HANDLER`. In Qt wird dies über das Event-System (`mousePressEvent`, etc.) gelöst.

#### 5. Empfehlung für die Portierung auf Qt
1. **Phase 1 (Geometrie):** `mathlib` und `main` (Modell) als plattformunabhängige Library kompilieren.
2. **Phase 2 (Rendering):** Eine neue `TDGraphicEngineQt` implementieren, die auf `QPainter` basiert.
3. **Phase 4 (View/Input):** Erstellung eines `QVEDWidget` (erbt von `QWidget`), das die Ereignisse an den `TDViewOperationManager` weiterleitet (Mapping von Qt-Events auf `TDOPVirtKey`).
4. **Phase 5 (Ressourcen):** Konvertierung der `.cur` und `.bmp` Dateien in Qt-kompatible Formate (z.B. `.png` für Icons, Pixmaps für Cursor).

Die saubere Trennung der Operationen von der UI ist eine hervorragende Basis für die Wiederbelebung.
