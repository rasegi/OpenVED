# Technische Notiz: Kommunikation zwischen Qt-UI und VED-Kern

Datum: 2026-05-16
Status: Analyse

## Kurzfassung

Die Kommunikation zwischen Qt-UI und VED-Kern laeuft aktuell nicht ueber Qt
Signals/Slots. Der VED-Kern bleibt Qt-frei.

Es gibt zwei Hauptwege:

- Core -> UI: eigenes VED-View-Interface `TDVecViewInterfaceBase`
- UI -> Core: `std::function`-Callbacks im `QVedWidget`, verdrahtet durch `MainWindow`

Qt Signals/Slots werden nur innerhalb der Qt-UI verwendet, z.B. fuer `QAction`,
`QScrollBar`, `QDockWidget`, `QLineEdit` und Buttons.

## Core -> UI

Der VED-Kern kennt keine Qt-Klassen wie `QWidget`, `QObject` oder `QCursor`.
Stattdessen kennt er nur das Interface:

```cpp
class TDVecViewInterfaceBase {
public:
    virtual void Send_ViewRefresh_ToView() = 0;
    virtual void Send_UseCursor_ToView(TDVecViewCursor eShape) = 0;
    virtual double Send_GetZoom_ToView() const = 0;
    virtual void Send_SetZoom_ToView(double nZoom, const POINT* pPoint) = 0;
    virtual bool Send_GetGridLock_ToView() const = 0;
    virtual bool Send_IsRectVisible_ToView(TDMatRect MatRect) const = 0;
    virtual long Send_GetMouseTolerance_ToView() const = 0;
    virtual double Send_GetMinimumDistance_ToView() const = 0;
    virtual void Send_ZoomInOutEvent_ToView(TDVOPZoomInOutExtVar* pExtVar) const = 0;
};
```

Datei:

- `src/ved_core/main/vec_view_interface.h`

Das ist die zentrale Adapter-Grenze vom Core zur View.

## Qt-Implementierung des View-Interfaces

`QVedWidget` besitzt ein Template-Interface:

```cpp
TDVecViewInterfaceTemplate<QVedWidget> viewInterface_;
```

Im Konstruktor von `QVedWidget` werden konkrete Qt-Methoden registriert:

```cpp
viewInterface_.Register(this);
viewInterface_.SetGraphicEngine(&graphicEngine_);
viewInterface_.Send_ViewRefresh_ToView(&QVedWidget::requestViewRefresh);
viewInterface_.Send_UseCursor_ToView(&QVedWidget::useCursor);
viewInterface_.Send_GetZoom_ToView(&QVedWidget::currentZoom);
viewInterface_.Send_SetZoom_ToView(&QVedWidget::setZoom);
viewInterface_.Send_GetGridLock_ToView(&QVedWidget::gridLock);
viewInterface_.Send_IsRectVisible_ToView(&QVedWidget::isRectVisible);
viewInterface_.Send_GetMouseTolerance_ToView(&QVedWidget::mouseTolerance);
viewInterface_.Send_GetMinimumDistance_ToView(&QVedWidget::minimumDistance);
viewInterface_.Send_ZoomInOutEvent_ToView(&QVedWidget::zoomInOutEvent);
```

Dateien:

- `src/app/QVedWidget.h`
- `src/app/QVedWidget.cpp`

Das ist kein Qt-Signal/Slot-System. Es ist ein eigenes C++-Callback-System mit
Member-Function-Pointern.

## UI -> Core

Die Gegenrichtung laeuft ueber `std::function`-Callbacks im `QVedWidget`.

`QVedWidget` definiert:

```cpp
using PaintContentCallback = std::function<void(TDVecViewInterfaceBase*)>;
using OperationMouseCallback = std::function<void(TDOPVirtMouseButton, TDOPVirtKeyState, TDMatPoint)>;
using MouseCoordinateCallback = std::function<void(TDMatPoint, bool)>;
```

und Setter:

```cpp
void setPaintContentCallback(PaintContentCallback callback);
void setOperationMouseDownCallback(OperationMouseCallback callback);
void setOperationMouseUpCallback(OperationMouseCallback callback);
void setOperationMouseMoveCallback(OperationMouseCallback callback);
void setMouseCoordinateCallback(MouseCoordinateCallback callback);
```

`MainWindow` verdrahtet diese Callbacks mit dem VED-Kern:

```cpp
canvas_->setPaintContentCallback([this](TDVecViewInterfaceBase* view) {
    editor_->PaintContentOnView(view);
});

canvas_->setOperationMouseDownCallback([this](TDOPVirtMouseButton button, TDOPVirtKeyState shift, TDMatPoint point) {
    editor_->MouseDown(canvas_->viewInterface(), button, shift, point.x, point.y);
});

canvas_->setOperationMouseUpCallback([this](TDOPVirtMouseButton button, TDOPVirtKeyState shift, TDMatPoint point) {
    editor_->MouseUp(canvas_->viewInterface(), button, shift, point.x, point.y);
});

canvas_->setOperationMouseMoveCallback([this](TDOPVirtMouseButton button, TDOPVirtKeyState shift, TDMatPoint point) {
    editor_->MouseMove(canvas_->viewInterface(), button, shift, point.x, point.y);
});
```

Dateien:

- `src/app/QVedWidget.h`
- `src/app/QVedWidget.cpp`
- `src/app/MainWindow.cpp`
- `src/ved_core/main/vec_edit_cad.h`
- `src/ved_core/main/vec_edit_cad.cpp`

## Mouse-Event-Fluss

Der typische Mouse-Move-Fluss ist:

```text
QVedWidget::mouseMoveEvent(...)
  -> operationMouseMoveCallback_(...)
    -> MainWindow callback
      -> TDVecEditCad::MouseMove(...)
        -> TDViewOperationManager::MouseMove(...)
          -> aktive Operation: OPMouseMove(...)
```

Wenn die Operation danach die UI beeinflussen muss, z.B. Cursor setzen oder View
refreshen, geht sie ueber das View-Interface zurueck:

```text
aktive Operation
  -> TDVecEditCad::UseCursor(...)
    -> TDVecViewInterfaceBase::Send_UseCursor_ToView(...)
      -> QVedWidget::useCursor(...)
```

## Paint-Fluss

Der Paint-Fluss ist ebenfalls callback-basiert:

```text
QVedWidget::paintEvent(...)
  -> paintContentCallback_(&viewInterface_)
    -> MainWindow callback
      -> TDVecEditCad::PaintContentOnView(view)
        -> Core zeichnet ueber TDGraphicEngine
          -> TDGraphicEngineQt fuehrt Qt-Zeichenoperationen aus
```

Die konkrete Qt-Zeichenanbindung liegt in:

- `src/ved_qt/gengine/vec_graphic_engine_qt.h`
- `src/ved_qt/gengine/vec_graphic_engine_qt.cpp`

## Qt Signals/Slots innerhalb der UI

Innerhalb der Qt-UI werden Qt Signals/Slots normal verwendet, z.B.:

- `QAction::triggered`
- `QAction::toggled`
- `QToolButton::clicked`
- `QLineEdit::returnPressed`
- `QDockWidget::visibilityChanged`
- `QScrollBar::valueChanged`
- `QScrollBar::actionTriggered`

Diese Verbindungen liegen hauptsaechlich in:

- `src/app/MainWindow.cpp`
- `src/app/QVedWidget.cpp`

Aktuell definiert die Anwendung aber keine eigenen Qt-Signale:

- kein eigenes `signals:`
- keine eigenen `public slots:` oder `private slots:`
- kein eigenes `emit`
- aktuell kein `Q_OBJECT` in `MainWindow` oder `QVedWidget`

## Bewertung

Die aktuelle Trennung ist fuer die Portierung grundsaetzlich sinnvoll:

- Der VED-Kern bleibt Qt-frei.
- Die Qt-Abhaengigkeit bleibt in `src/app` und `src/ved_qt`.
- Alte VED-Mechanik wie `TDVecViewInterfaceBase` bleibt erkennbar.
- Der Core kann theoretisch auch mit einer anderen View-Implementierung arbeiten.

Die groesste architektonische Schwaeche liegt nicht in der Core/UI-Grenze selbst,
sondern darin, dass `MainWindow` aktuell sehr viele Rollen kombiniert:

- App-Shell
- Toolbar- und Menu-Aufbau
- Callback-Verdrahtung zwischen Canvas und Core
- UI-State-Regeln fuer aktive Operationen
- Dock-Logik
- Cursor-Policy
- Statuszeile

Eine spaetere Modularisierung sollte daher eher `MainWindow` entlasten, nicht den
VED-Kern an Qt koppeln.

## Empfehlung

Der VED-Kern sollte weiterhin nicht von `QObject`, Qt Signals/Slots oder Qt-Widgets
abhaengen.

Sinnvolle Richtung fuer spaetere Refaktorierung:

- `TDVecViewInterfaceBase` als Core/View-Grenze erhalten oder gezielt modernisieren.
- `QVedWidget` als Qt-Adapter zur View beibehalten.
- `MainWindow` in kleinere UI-Komponenten aufteilen.
- Qt Signals/Slots nur innerhalb der Qt-UI verwenden.
- Neue UI-Komponenten duerfen Qt-idiomatisch sein.
- Core-nahe Klassen sollten weiter Qt-frei bleiben.
