#pragma once

#include "vec_graphic_engine_qt.h"
#include "vec_view_interface.h"
#include "ved_document_view_state.h"

#include <QWidget>

#include <functional>

class QKeyEvent;
class QEvent;
class QMouseEvent;
class QPoint;
class QPaintEvent;
class QRect;
class QResizeEvent;
class QScrollBar;
class QWheelEvent;
class TDVOPZoomInOutExtVar;

class QVedWidget final : public QWidget {
public:
    enum class InteractionMode {
        None,
        CreateLine,
        CreateText,
        SelectObject,
        MoveObject,
        DeleteObject,
        ModifyObject
    };

    using PaintContentCallback = std::function<void(TDVecViewInterfaceBase*)>;
    using OperationMouseCallback = std::function<void(TDOPVirtMouseButton, TDOPVirtKeyState, TDMatPoint)>;
    using MouseCoordinateCallback = std::function<void(TDMatPoint, bool)>;

    explicit QVedWidget(QWidget* parent = nullptr);

    TDVecViewInterfaceBase* viewInterface();
    TDGraphicEngineQt* graphicEngine();
    void setPaintContentCallback(PaintContentCallback callback);
    void setOperationMouseDownCallback(OperationMouseCallback callback);
    void setOperationMouseUpCallback(OperationMouseCallback callback);
    void setOperationMouseMoveCallback(OperationMouseCallback callback);
    void setMouseCoordinateCallback(MouseCoordinateCallback callback);
    void setInteractionMode(InteractionMode mode);
    InteractionMode interactionMode() const;
    void setShowGrid(bool showGrid);
    bool showGrid() const;
    void setShowRulers(bool showRulers);
    bool showRulers() const;
    void setGridLock(bool gridLock);
    bool gridLock() const;
    void setShowMouseToleranceCross(bool showCross);
    bool showMouseToleranceCross() const;
    void setZoomToolEnabled(bool enabled);
    bool zoomToolEnabled() const;
    void setPanToolEnabled(bool enabled);
    bool panToolEnabled() const;
    VEDDocumentViewState documentViewState();
    void applyDocumentViewState(const VEDDocumentViewState& viewState);
    void resetView();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void requestViewRefresh();
    void refreshDeviceMetrics();
    void initializeViewIfNeeded();
    QRect canvasRect() const;
    void updateScrollBars();
    void setPanFromScrollBars();
    void handleHorizontalScrollAction(int action);
    void handleVerticalScrollAction(int action);
    int horizontalLineStep() const;
    int verticalLineStep() const;
    int horizontalPageScrollStep() const;
    int verticalPageScrollStep() const;
    void drawGrid();
    void drawRulers();
    void drawAxes();
    void zoomAtScreenPoint(const QPoint& screenPoint, double factor);
    void useCursor(TDVecViewCursor shape);
    double currentZoom() const;
    void setZoom(double zoom, const POINT* point);
    bool isRectVisible(TDMatRect rect) const;
    long mouseTolerance() const;
    double minimumDistance() const;
    void zoomInOutEvent(TDVOPZoomInOutExtVar* extVar) const;
    TDMatPoint screenToRealPoint(const QPoint& screenPoint) const;
    TDMatPoint snapPointToGrid(TDMatPoint point) const;
    TDOPVirtMouseButton virtualButton(Qt::MouseButton button) const;
    TDOPVirtMouseButton virtualButtons(Qt::MouseButtons buttons) const;
    TDOPVirtKeyState virtualKeyState(Qt::KeyboardModifiers modifiers) const;
    TDMatPoint operationPoint(const QPoint& screenPoint) const;
    void updateSnapMousePoint(const QPoint& screenPoint);
    bool shouldDrawMouseToleranceMarker() const;
    bool shouldDrawMouseToleranceCross() const;
    void notifyMouseCoordinate(const QPoint& screenPoint);
    void drawSnapMouseMarker();
    void drawSelectionAreaMarker();

    TDGraphicEngineQt graphicEngine_;
    TDVecViewInterfaceTemplate<QVedWidget> viewInterface_;
    PaintContentCallback paintContentCallback_;
    OperationMouseCallback operationMouseDownCallback_;
    OperationMouseCallback operationMouseUpCallback_;
    OperationMouseCallback operationMouseMoveCallback_;
    MouseCoordinateCallback mouseCoordinateCallback_;
    QScrollBar* horizontalScrollBar_;
    QScrollBar* verticalScrollBar_;
    QPoint lastPanPoint_;
    InteractionMode interactionMode_;
    TDVecViewCursor activeCursor_;
    TDMatPoint snapMousePoint_;
    TDMatPoint selectionAreaStart_;
    TDMatPoint selectionAreaCurrent_;
    bool showGrid_;
    bool showRulers_;
    bool gridLock_;
    bool showMouseToleranceCross_;
    bool zoomToolEnabled_;
    bool panToolEnabled_;
    bool viewInitialized_;
    bool panning_;
    bool updatingScrollBars_;
    bool hasSnapMousePoint_;
    bool selectionAreaActive_;
};
