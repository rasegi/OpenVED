#include "QVedWidget.h"

#include "vec_measure_scale.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPoint>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStyle>
#include <QWheelEvent>
#include <Qt>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr double kDefaultWorkspaceWidth = 210000.0;
constexpr double kDefaultWorkspaceHeight = 297000.0;
constexpr double kDocumentMargin = 25000.0;
constexpr int kMaxGridPointsPerPaint = 80000;
constexpr long kMaxGridStep = 1000000000;
constexpr int kPageScrollOverlapPerThousand = 100;
constexpr int kScrollsPerPage = 8;
constexpr TDRgbColor kSubGridColor = 0x00808080;
constexpr TDRgbColor kMainGridColor = 0x00282828;
constexpr TDRgbColor kAxisColor = 0x00000000;

constexpr double kDefaultGridMajorStep = 10000.0;
constexpr int kDefaultGridSubdivisions = 10;
constexpr long kDefaultGridResolutionLimit = 1;

bool isGridMainPoint(long x, long y, long mainStep) {
    if (mainStep <= 0) {
        return false;
    }

    return x % mainStep == 0 && y % mainStep == 0;
}

bool forwardsOperationMouseEvents(QVedWidget::InteractionMode mode) {
    return mode == QVedWidget::InteractionMode::CreateLine ||
           mode == QVedWidget::InteractionMode::CreateText ||
           mode == QVedWidget::InteractionMode::SelectObject ||
           mode == QVedWidget::InteractionMode::MoveObject ||
           mode == QVedWidget::InteractionMode::DeleteObject ||
           mode == QVedWidget::InteractionMode::ModifyObject;
}
} // namespace

QVedWidget::QVedWidget(QWidget* parent)
    : QWidget(parent),
      horizontalScrollBar_(new QScrollBar(Qt::Horizontal, this)),
      verticalScrollBar_(new QScrollBar(Qt::Vertical, this)),
      interactionMode_(InteractionMode::None),
      activeCursor_(VECVIEW_CURSOR_DEFAULT),
      snapMousePoint_{0.0, 0.0},
      selectionAreaStart_{0.0, 0.0},
      selectionAreaCurrent_{0.0, 0.0},
      documentSettings_(nullptr),
      showGrid_(false),
      showRulers_(false),
      gridLock_(false),
      showMouseToleranceCross_(false),
      zoomToolEnabled_(false),
      panToolEnabled_(false),
      viewInitialized_(false),
      panning_(false),
      updatingScrollBars_(false),
      hasSnapMousePoint_(false),
      selectionAreaActive_(false) {
    setObjectName(QStringLiteral("VedCanvas"));
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(false);

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

    connect(horizontalScrollBar_, &QScrollBar::valueChanged, this, [this](int) {
        setPanFromScrollBars();
    });
    connect(verticalScrollBar_, &QScrollBar::valueChanged, this, [this](int) {
        setPanFromScrollBars();
    });
    connect(horizontalScrollBar_, &QScrollBar::actionTriggered, this, &QVedWidget::handleHorizontalScrollAction);
    connect(verticalScrollBar_, &QScrollBar::actionTriggered, this, &QVedWidget::handleVerticalScrollAction);
}

TDVecViewInterfaceBase* QVedWidget::viewInterface() {
    return &viewInterface_;
}

TDGraphicEngineQt* QVedWidget::graphicEngine() {
    return &graphicEngine_;
}

void QVedWidget::setPaintContentCallback(PaintContentCallback callback) {
    paintContentCallback_ = std::move(callback);
    update();
}

void QVedWidget::setOperationMouseDownCallback(OperationMouseCallback callback) {
    operationMouseDownCallback_ = std::move(callback);
}

void QVedWidget::setOperationMouseUpCallback(OperationMouseCallback callback) {
    operationMouseUpCallback_ = std::move(callback);
}

void QVedWidget::setOperationMouseMoveCallback(OperationMouseCallback callback) {
    operationMouseMoveCallback_ = std::move(callback);
}

void QVedWidget::setMouseCoordinateCallback(MouseCoordinateCallback callback) {
    mouseCoordinateCallback_ = std::move(callback);
}

void QVedWidget::setInteractionMode(InteractionMode mode) {
    if (interactionMode_ == mode) {
        return;
    }

    interactionMode_ = mode;
    switch (interactionMode_) {
    case InteractionMode::CreateLine:
        break;
    case InteractionMode::CreateText:
        useCursor(VECVIEW_CREATE_VECTEXT_1);
        break;
    case InteractionMode::SelectObject:
        useCursor(VECVIEW_SELECT_OBJECT);
        break;
    case InteractionMode::MoveObject:
        useCursor(VECVIEW_DOCK);
        break;
    case InteractionMode::DeleteObject:
        useCursor(VECVIEW_DELETE_OBJECT);
        break;
    case InteractionMode::ModifyObject:
        break;
    case InteractionMode::None:
        useCursor(zoomToolEnabled_ ? VECVIEW_ZOOM : (panToolEnabled_ ? VECVIEW_DOCK : VECVIEW_CURSOR_DEFAULT));
        break;
    }
    update();
}

QVedWidget::InteractionMode QVedWidget::interactionMode() const {
    return interactionMode_;
}

void QVedWidget::setDocumentSettings(const TDVecDocumentSettings* settings) {
    documentSettings_ = settings;
}

void QVedWidget::setShowGrid(bool showGrid) {
    if (showGrid_ == showGrid) {
        return;
    }
    showGrid_ = showGrid;
    update();
}

bool QVedWidget::showGrid() const {
    return showGrid_;
}

void QVedWidget::setShowRulers(bool showRulers) {
    if (showRulers_ == showRulers) {
        return;
    }
    showRulers_ = showRulers;
    update();
}

bool QVedWidget::showRulers() const {
    return showRulers_;
}

void QVedWidget::setGridLock(bool gridLock) {
    gridLock_ = gridLock;
    hasSnapMousePoint_ = false;
    update();
}

bool QVedWidget::gridLock() const {
    return gridLock_;
}

void QVedWidget::setShowMouseToleranceCross(bool showCross) {
    if (showMouseToleranceCross_ == showCross) {
        return;
    }
    showMouseToleranceCross_ = showCross;
    hasSnapMousePoint_ = false;
    update();
}

bool QVedWidget::showMouseToleranceCross() const {
    return showMouseToleranceCross_;
}

void QVedWidget::setZoomToolEnabled(bool enabled) {
    zoomToolEnabled_ = enabled;
    if (enabled) {
        panToolEnabled_ = false;
    }
    if (enabled || interactionMode_ == InteractionMode::None) {
        useCursor(enabled ? VECVIEW_ZOOM : (panToolEnabled_ ? VECVIEW_DOCK : VECVIEW_CURSOR_DEFAULT));
    }
}

bool QVedWidget::zoomToolEnabled() const {
    return zoomToolEnabled_;
}

void QVedWidget::setPanToolEnabled(bool enabled) {
    panToolEnabled_ = enabled;
    if (enabled) {
        zoomToolEnabled_ = false;
    }
    if (enabled || interactionMode_ == InteractionMode::None) {
        useCursor(enabled ? VECVIEW_DOCK : (zoomToolEnabled_ ? VECVIEW_ZOOM : VECVIEW_CURSOR_DEFAULT));
    }
}

bool QVedWidget::panToolEnabled() const {
    return panToolEnabled_;
}

VEDDocumentViewState QVedWidget::documentViewState() {
    refreshDeviceMetrics();
    initializeViewIfNeeded();

    const TDCoordinateSystem2D& viewRange = graphicEngine_.GetViewRange();
    VEDDocumentViewState viewState;
    viewState.present = true;
    viewState.zoom = graphicEngine_.GetZoom();
    viewState.center = {
        (viewRange.Left + viewRange.Right) / 2.0,
        (viewRange.Top + viewRange.Bottom) / 2.0
    };
    viewState.showGrid = showGrid_;
    viewState.gridLock = gridLock_;
    viewState.showRulers = showRulers_;
    return viewState;
}

void QVedWidget::applyDocumentViewState(const VEDDocumentViewState& viewState) {
    if (!viewState.present) {
        resetView();
        return;
    }

    refreshDeviceMetrics();
    initializeViewIfNeeded();

    showGrid_ = viewState.showGrid;
    gridLock_ = viewState.gridLock;
    showRulers_ = viewState.showRulers;
    hasSnapMousePoint_ = false;
    selectionAreaActive_ = false;

    const double zoom = std::isfinite(viewState.zoom)
        ? std::clamp(viewState.zoom, 0.02, 80.0)
        : 1.0;
    graphicEngine_.SetZoom(zoom);

    const TDCoordinateSystem2D zoomedRange = graphicEngine_.GetViewRange();
    const double width = std::max(1.0, std::fabs(zoomedRange.Right - zoomedRange.Left));
    const double height = std::max(1.0, std::fabs(zoomedRange.Bottom - zoomedRange.Top));
    const double centerX = std::isfinite(viewState.center.x) ? viewState.center.x : 0.0;
    const double centerY = std::isfinite(viewState.center.y) ? viewState.center.y : 0.0;
    const double left = centerX - width / 2.0;
    const double right = centerX + width / 2.0;
    const double top = centerY - height / 2.0;
    const double bottom = centerY + height / 2.0;

    const TDCoordinateSystem2D& worldRange = graphicEngine_.GetWorldSpaceRange();
    graphicEngine_.SetWorldSpaceRange(
        std::min(worldRange.Left, left),
        std::min(worldRange.Top, top),
        std::max(worldRange.Right, right),
        std::max(worldRange.Bottom, bottom));
    graphicEngine_.SetViewRange(left, top, right, bottom);
    viewInitialized_ = true;
    updateScrollBars();
    update();
}

void QVedWidget::resetView() {
    viewInitialized_ = false;
    panning_ = false;
    selectionAreaActive_ = false;
    hasSnapMousePoint_ = false;
    refreshDeviceMetrics();
    initializeViewIfNeeded();
    update();
}

void QVedWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(232, 232, 232));
    const QRect viewRect = canvasRect();
    painter.fillRect(viewRect, QColor(250, 250, 248));
    painter.setClipRect(viewRect);

    graphicEngine_.SetDeviceMetrics(
        viewRect.width(),
        viewRect.height(),
        painter.device()->logicalDpiX(),
        painter.device()->logicalDpiY());
    initializeViewIfNeeded();
    graphicEngine_.SetPainter(&painter);
    drawGrid();
    drawAxes();
    if (paintContentCallback_) {
        paintContentCallback_(&viewInterface_);
    }
    drawSelectionAreaMarker();
    drawSnapMouseMarker();
    drawRulers();
    graphicEngine_.SetPainter(nullptr);
    painter.setClipping(false);
}

void QVedWidget::mousePressEvent(QMouseEvent* event) {
    notifyMouseCoordinate(event->pos());
    setFocus(Qt::MouseFocusReason);
    if (!canvasRect().contains(event->pos())) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (panToolEnabled_ && event->button() == Qt::LeftButton) {
        panning_ = true;
        lastPanPoint_ = event->pos();
        useCursor(VECVIEW_DOCK);
        event->accept();
        return;
    }

    if (!panToolEnabled_ && forwardsOperationMouseEvents(interactionMode_)) {
        if (interactionMode_ == InteractionMode::SelectObject && event->button() == Qt::LeftButton) {
            selectionAreaStart_ = operationPoint(event->pos());
            selectionAreaCurrent_ = selectionAreaStart_;
            selectionAreaActive_ = true;
        }
        if (operationMouseDownCallback_) {
            operationMouseDownCallback_(virtualButton(event->button()), virtualKeyState(event->modifiers()), operationPoint(event->pos()));
            event->accept();
            return;
        }
    }

    if (zoomToolEnabled_) {
        if (event->button() == Qt::LeftButton) {
            zoomAtScreenPoint(event->pos(), 1.4);
            event->accept();
            return;
        }
        if (event->button() == Qt::RightButton) {
            zoomAtScreenPoint(event->pos(), 1.0 / 1.4);
            event->accept();
            return;
        }
    }

    if (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton) {
        panning_ = true;
        lastPanPoint_ = event->pos();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void QVedWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    mousePressEvent(event);
}

void QVedWidget::mouseMoveEvent(QMouseEvent* event) {
    notifyMouseCoordinate(event->pos());
    updateSnapMousePoint(event->pos());

    if (forwardsOperationMouseEvents(interactionMode_) && canvasRect().contains(event->pos())) {
        if (selectionAreaActive_) {
            selectionAreaCurrent_ = operationPoint(event->pos());
            update();
        }
        if (operationMouseMoveCallback_) {
            operationMouseMoveCallback_(virtualButtons(event->buttons()), virtualKeyState(event->modifiers()), operationPoint(event->pos()));
        }
        event->accept();
        return;
    }

    if (panning_) {
        const QPoint diff = event->pos() - lastPanPoint_;
        graphicEngine_.SetPanAllowingWorldExpansion(
            graphicEngine_.GetPanX() - diff.x(),
            graphicEngine_.GetPanY() - diff.y());
        lastPanPoint_ = event->pos();
        updateScrollBars();
        update();
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void QVedWidget::mouseReleaseEvent(QMouseEvent* event) {
    notifyMouseCoordinate(event->pos());
    if (forwardsOperationMouseEvents(interactionMode_) && canvasRect().contains(event->pos())) {
        if (selectionAreaActive_ && event->button() == Qt::LeftButton) {
            selectionAreaCurrent_ = operationPoint(event->pos());
            selectionAreaActive_ = false;
            update();
        }
        if (operationMouseUpCallback_) {
            operationMouseUpCallback_(virtualButton(event->button()), virtualKeyState(event->modifiers()), operationPoint(event->pos()));
        }
        event->accept();
        return;
    }

    if (panning_ && (event->button() == Qt::MiddleButton || event->button() == Qt::RightButton)) {
        panning_ = false;
        event->accept();
        return;
    }
    if (panning_ && panToolEnabled_ && event->button() == Qt::LeftButton) {
        panning_ = false;
        useCursor(VECVIEW_DOCK);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void QVedWidget::leaveEvent(QEvent* event) {
    if (mouseCoordinateCallback_) {
        mouseCoordinateCallback_({0.0, 0.0}, false);
    }
    if (hasSnapMousePoint_) {
        hasSnapMousePoint_ = false;
        update();
    }
    QWidget::leaveEvent(event);
}

void QVedWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);

    const int extent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this);
    horizontalScrollBar_->setGeometry(0, height() - extent, std::max(0, width() - extent), extent);
    verticalScrollBar_->setGeometry(width() - extent, 0, extent, std::max(0, height() - extent));
    refreshDeviceMetrics();
    updateScrollBars();
}

void QVedWidget::wheelEvent(QWheelEvent* event) {
    const int delta = event->angleDelta().y();
    if (delta == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    const QPoint point = event->position().toPoint();
    if (!canvasRect().contains(point)) {
        QWidget::wheelEvent(event);
        return;
    }

    zoomAtScreenPoint(point, delta > 0 ? 1.15 : 1.0 / 1.15);
    event->accept();
}

void QVedWidget::keyPressEvent(QKeyEvent* event) {
    QWidget::keyPressEvent(event);
}

void QVedWidget::keyReleaseEvent(QKeyEvent* event) {
    QWidget::keyReleaseEvent(event);
}

void QVedWidget::requestViewRefresh() {
    update();
}

void QVedWidget::refreshDeviceMetrics() {
    const QRect viewRect = canvasRect();
    if (viewRect.width() <= 1 || viewRect.height() <= 1) {
        return;
    }

    graphicEngine_.SetDeviceMetrics(
        viewRect.width(),
        viewRect.height(),
        logicalDpiX(),
        logicalDpiY());
}

void QVedWidget::initializeViewIfNeeded() {
    const QRect viewRect = canvasRect();
    if (viewInitialized_ || viewRect.width() <= 1 || viewRect.height() <= 1) {
        return;
    }

    const double pageWidth = documentSettings_ ? documentSettings_->pageSettings.widthReal : kDefaultWorkspaceWidth;
    const double pageHeight = documentSettings_ ? documentSettings_->pageSettings.heightReal : kDefaultWorkspaceHeight;
    graphicEngine_.SetWorkSpaceRange(0.0, 0.0, pageWidth, pageHeight);
    graphicEngine_.SetWorldSpaceRange(
        -kDocumentMargin,
        -kDocumentMargin,
        pageWidth + kDocumentMargin,
        pageHeight + kDocumentMargin);
    graphicEngine_.SetViewRange(
        -kDocumentMargin,
        -kDocumentMargin,
        pageWidth + kDocumentMargin,
        pageHeight + kDocumentMargin);
    viewInitialized_ = true;
    updateScrollBars();
}

QRect QVedWidget::canvasRect() const {
    const int extent = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, const_cast<QVedWidget*>(this));
    return {0, 0, std::max(1, width() - extent), std::max(1, height() - extent)};
}

void QVedWidget::updateScrollBars() {
    if (updatingScrollBars_) {
        return;
    }

    const QRect viewRect = canvasRect();
    if (viewRect.width() <= 1 || viewRect.height() <= 1) {
        return;
    }

    const TDCoordinateSystem2D& viewRange = graphicEngine_.GetViewRange();
    const TDCoordinateSystem2D& worldRange = graphicEngine_.GetWorldSpaceRange();
    const double realPerPixelX = (viewRange.Right - viewRange.Left) / static_cast<double>(viewRect.width());
    const double realPerPixelY = (viewRange.Bottom - viewRange.Top) / static_cast<double>(viewRect.height());

    const int totalWidth = std::max(
        viewRect.width(),
        static_cast<int>(MatRound(std::fabs((worldRange.Right - worldRange.Left) / realPerPixelX))));
    const int totalHeight = std::max(
        viewRect.height(),
        static_cast<int>(MatRound(std::fabs((worldRange.Bottom - worldRange.Top) / realPerPixelY))));

    updatingScrollBars_ = true;
    horizontalScrollBar_->setPageStep(viewRect.width());
    horizontalScrollBar_->setSingleStep(horizontalLineStep());
    horizontalScrollBar_->setRange(0, std::max(0, totalWidth - viewRect.width()));
    const int clampedPanX = std::clamp<int>(graphicEngine_.GetPanX(), 0, horizontalScrollBar_->maximum());
    if (clampedPanX != graphicEngine_.GetPanX()) {
        graphicEngine_.SetPanX(clampedPanX);
    }
    horizontalScrollBar_->setValue(clampedPanX);

    verticalScrollBar_->setPageStep(viewRect.height());
    verticalScrollBar_->setSingleStep(verticalLineStep());
    verticalScrollBar_->setRange(0, std::max(0, totalHeight - viewRect.height()));
    const int clampedPanY = std::clamp<int>(graphicEngine_.GetPanY(), 0, verticalScrollBar_->maximum());
    if (clampedPanY != graphicEngine_.GetPanY()) {
        graphicEngine_.SetPanY(clampedPanY);
    }
    verticalScrollBar_->setValue(clampedPanY);
    updatingScrollBars_ = false;
}

void QVedWidget::setPanFromScrollBars() {
    if (updatingScrollBars_) {
        return;
    }

    graphicEngine_.SetPanX(horizontalScrollBar_->value());
    graphicEngine_.SetPanY(verticalScrollBar_->value());
    update();
}

void QVedWidget::handleHorizontalScrollAction(int action) {
    if (updatingScrollBars_) {
        return;
    }

    int value = horizontalScrollBar_->value();
    switch (action) {
    case QAbstractSlider::SliderSingleStepSub:
        value -= horizontalLineStep();
        break;
    case QAbstractSlider::SliderSingleStepAdd:
        value += horizontalLineStep();
        break;
    case QAbstractSlider::SliderPageStepSub:
        value -= horizontalPageScrollStep();
        break;
    case QAbstractSlider::SliderPageStepAdd:
        value += horizontalPageScrollStep();
        break;
    case QAbstractSlider::SliderToMinimum:
        value = horizontalScrollBar_->minimum();
        break;
    case QAbstractSlider::SliderToMaximum:
        value = horizontalScrollBar_->maximum();
        break;
    case QAbstractSlider::SliderMove:
        value = horizontalScrollBar_->sliderPosition();
        break;
    default:
        return;
    }

    horizontalScrollBar_->setSliderPosition(std::clamp(value, horizontalScrollBar_->minimum(), horizontalScrollBar_->maximum()));
}

void QVedWidget::handleVerticalScrollAction(int action) {
    if (updatingScrollBars_) {
        return;
    }

    int value = verticalScrollBar_->value();
    switch (action) {
    case QAbstractSlider::SliderSingleStepSub:
        value -= verticalLineStep();
        break;
    case QAbstractSlider::SliderSingleStepAdd:
        value += verticalLineStep();
        break;
    case QAbstractSlider::SliderPageStepSub:
        value -= verticalPageScrollStep();
        break;
    case QAbstractSlider::SliderPageStepAdd:
        value += verticalPageScrollStep();
        break;
    case QAbstractSlider::SliderToMinimum:
        value = verticalScrollBar_->minimum();
        break;
    case QAbstractSlider::SliderToMaximum:
        value = verticalScrollBar_->maximum();
        break;
    case QAbstractSlider::SliderMove:
        value = verticalScrollBar_->sliderPosition();
        break;
    default:
        return;
    }

    verticalScrollBar_->setSliderPosition(std::clamp(value, verticalScrollBar_->minimum(), verticalScrollBar_->maximum()));
}

int QVedWidget::horizontalLineStep() const {
    return std::max(1, horizontalPageScrollStep() / kScrollsPerPage);
}

int QVedWidget::verticalLineStep() const {
    return std::max(1, verticalPageScrollStep() / kScrollsPerPage);
}

int QVedWidget::horizontalPageScrollStep() const {
    const QRect viewRect = canvasRect();
    const int overlap = viewRect.width() * kPageScrollOverlapPerThousand / 1000;
    return std::max(1, viewRect.width() - overlap);
}

int QVedWidget::verticalPageScrollStep() const {
    const QRect viewRect = canvasRect();
    const int overlap = viewRect.height() * kPageScrollOverlapPerThousand / 1000;
    return std::max(1, viewRect.height() - overlap);
}

void QVedWidget::drawGrid() {
    if (!showGrid_) {
        return;
    }

    const TDCoordinateSystem2D& viewRange = graphicEngine_.GetViewRange();
    const double left = std::min(viewRange.Left, viewRange.Right);
    const double right = std::max(viewRange.Left, viewRange.Right);
    const double top = std::min(viewRange.Top, viewRange.Bottom);
    const double bottom = std::max(viewRange.Top, viewRange.Bottom);
    const double visibleWidth = std::max(1.0, right - left);
    const double visibleHeight = std::max(1.0, bottom - top);

    const double gridMajorStep = documentSettings_ ? documentSettings_->gridSettings.majorStepReal : kDefaultGridMajorStep;
    const int gridSubdivisions = documentSettings_ ? documentSettings_->gridSettings.subdivisions : kDefaultGridSubdivisions;
    const long gridResLimit = documentSettings_ ? documentSettings_->gridSettings.resolutionLimitPixels : kDefaultGridResolutionLimit;

    const double minStepByPixel = std::max(
        std::fabs(graphicEngine_.ScreenToXVal(gridResLimit)),
        std::fabs(graphicEngine_.ScreenToYVal(gridResLimit)));
    const double minStepByPointBudget = std::sqrt((visibleWidth * visibleHeight) / kMaxGridPointsPerPaint);
    const double minorFromPreferred = (gridSubdivisions > 0)
        ? gridMajorStep / static_cast<double>(gridSubdivisions) : gridMajorStep;
    const double minimumStep = std::max({
        minorFromPreferred,
        minStepByPixel,
        minStepByPointBudget
    });
    const long gridStep = NiceStepAtLeast(minimumStep);

    const long firstX = static_cast<long>(std::floor(left / static_cast<double>(gridStep))) * gridStep;
    const long lastX = static_cast<long>(std::ceil(right / static_cast<double>(gridStep))) * gridStep;
    const long firstY = static_cast<long>(std::floor(top / static_cast<double>(gridStep))) * gridStep;
    const long lastY = static_cast<long>(std::ceil(bottom / static_cast<double>(gridStep))) * gridStep;
    const long columns = ((lastX - firstX) / gridStep) + 1;
    const long rows = ((lastY - firstY) / gridStep) + 1;

    if (gridStep > kMaxGridStep || columns <= 0 || rows <= 0 || columns * rows > kMaxGridPointsPerPaint) {
        return;
    }

    const long mainStep = std::max(static_cast<long>(gridMajorStep), gridStep);
    const bool drawSubGrid = gridStep < mainStep;
    const TDRgbColor oldColor = graphicEngine_.GetDrawColor();

    if (drawSubGrid) {
        graphicEngine_.SetDrawColor(kSubGridColor);
        for (long y = firstY; y <= lastY; y += gridStep) {
            for (long x = firstX; x <= lastX; x += gridStep) {
                if (isGridMainPoint(x, y, mainStep)) {
                    continue;
                }

                const TDMatLine pointLine{
                    static_cast<double>(x),
                    static_cast<double>(y),
                    static_cast<double>(x),
                    static_cast<double>(y)
                };
                graphicEngine_.DrawLine(&pointLine, false);
            }
        }
    }

    graphicEngine_.SetDrawColor(kMainGridColor);
    const double mainMarkerHalfWidth = std::fabs(graphicEngine_.ScreenToXVal(2));
    const double mainMarkerHalfHeight = std::fabs(graphicEngine_.ScreenToYVal(2));
    for (long y = firstY; y <= lastY; y += gridStep) {
        for (long x = firstX; x <= lastX; x += gridStep) {
            if (drawSubGrid && !isGridMainPoint(x, y, mainStep)) {
                continue;
            }

            const TDMatLine horizontalLine{
                static_cast<double>(x) - mainMarkerHalfWidth,
                static_cast<double>(y),
                static_cast<double>(x) + mainMarkerHalfWidth,
                static_cast<double>(y)
            };
            const TDMatLine verticalLine{
                static_cast<double>(x),
                static_cast<double>(y) - mainMarkerHalfHeight,
                static_cast<double>(x),
                static_cast<double>(y) + mainMarkerHalfHeight
            };
            graphicEngine_.DrawLine(&horizontalLine, false);
            graphicEngine_.DrawLine(&verticalLine, false);
        }
    }
    graphicEngine_.SetDrawColor(oldColor);
}

void QVedWidget::drawRulers() {
    if (!showRulers_) {
        return;
    }

    const TDVecUnitSettings unitSettings = documentSettings_
        ? documentSettings_->unitSettings : TDVecUnitSettings{};
    const double gridMajorStep = documentSettings_
        ? documentSettings_->gridSettings.majorStepReal : kDefaultGridMajorStep;
    const int gridSubdivisions = documentSettings_
        ? documentSettings_->gridSettings.subdivisions : kDefaultGridSubdivisions;
    const long gridResLimit = documentSettings_
        ? documentSettings_->gridSettings.resolutionLimitPixels : kDefaultGridResolutionLimit;

    const TDVecUnitFormatter formatter(unitSettings);
    const TDVecMeasureScaleCalculator calculator(unitSettings);
    const double realPerPixel = std::fabs(graphicEngine_.ScreenToXVal(1));
    const TDVecMeasureScale scale = calculator.CalculateForView(
        realPerPixel, gridResLimit, gridMajorStep, gridSubdivisions);

    graphicEngine_.DrawRulers(scale, formatter);
}

void QVedWidget::drawAxes() {
    const TDRgbColor oldColor = graphicEngine_.GetDrawColor();
    graphicEngine_.SetDrawColor(kAxisColor);
    graphicEngine_.DrawAuxLineX(0);
    graphicEngine_.DrawAuxLineY(0);
    graphicEngine_.SetDrawColor(oldColor);
}

void QVedWidget::zoomAtScreenPoint(const QPoint& screenPoint, double factor) {
    if (!canvasRect().contains(screenPoint)) {
        return;
    }

    refreshDeviceMetrics();
    initializeViewIfNeeded();

    const double oldZoom = graphicEngine_.GetZoom();
    const double newZoom = std::clamp(oldZoom * factor, 0.02, 80.0);
    if (std::fabs(newZoom - oldZoom) < 0.000001) {
        return;
    }

    graphicEngine_.SetZoomAtScreenPoint(newZoom, screenPoint.x(), screenPoint.y());

    updateScrollBars();
    update();
}

void QVedWidget::useCursor(TDVecViewCursor shape) {
    activeCursor_ = shape;

    auto useResourceCursor = [this](const QString& resourcePath, int hotX, int hotY) {
        const QPixmap pixmap(resourcePath);
        if (!pixmap.isNull()) {
            setCursor(QCursor(pixmap, hotX, hotY));
            return true;
        }
        return false;
    };

    switch (shape) {
    case VECVIEW_CURSOR_DEFAULT:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/select_object.png"), 7, 11)) {
            break;
        }
        unsetCursor();
        break;
    case VECVIEW_CREATE_LINE_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_line_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_LINE_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_line_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_ZOOM:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/zoom.png"), 9, 8)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_PLUS:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/plus.png"), 16, 16)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_NO:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/no.png"), 16, 15)) {
            break;
        }
        setCursor(Qt::ForbiddenCursor);
        break;
    case VECVIEW_SIMPLE_CROSS:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/crosshair_none.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CROSS_PLUS:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/crosshair_plus.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CROSS_MINUS:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/crosshair_minus.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_DOCK:
        if (panToolEnabled_ && useResourceCursor(QStringLiteral(":/ved/cursors/drag_document.png"), 8, 8)) {
            break;
        }
        if (useResourceCursor(QStringLiteral(":/ved/cursors/dock.png"), 16, 16)) {
            break;
        }
        setCursor(Qt::ClosedHandCursor);
        break;
    case VECVIEW_DELETE_OBJECT:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/delete_object.png"), 6, 6)) {
            break;
        }
        setCursor(Qt::PointingHandCursor);
        break;
    case VECVIEW_CREATE_CIRCLE_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_circle_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_CIRCLE_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_circle_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_ELLIPSE_ROTATED_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_ellipse_rotated_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_ELLIPSE_ROTATED_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_ellipse_rotated_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_ELLIPSE_NOTROTATED_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_ellipse_notrotated_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_ELLIPSE_NOTROTATED_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_ellipse_notrotated_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_POLYGON_SMARTLINE_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_polygon_smartline_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_POLYGON_SMARTLINE_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_polygon_smartline_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_POLYCURVE_SMARTLINE_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_polycurve_smartline_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_POLYCURVE_SMARTLINE_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_polycurve_smartline_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_RECTANGLE_ROTATED_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_rectangle_rotated_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_RECTANGLE_ROTATED_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_rectangle_rotated_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_RECTANGLE_NOTROTATED_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_rectangle_notrotated_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_RECTANGLE_NOTROTATED_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_rectangle_notrotated_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_BSPLINE_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_bspline_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_BSPLINE_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_bspline_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::CrossCursor);
        break;
    case VECVIEW_CREATE_VECTEXT_1:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_vectext_1.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::IBeamCursor);
        break;
    case VECVIEW_CREATE_VECTEXT_2:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/create_vectext_2.png"), 11, 11)) {
            break;
        }
        setCursor(Qt::IBeamCursor);
        break;
    case VECVIEW_SELECT_OBJECT:
        if (useResourceCursor(QStringLiteral(":/ved/cursors/select_object.png"), 7, 11)) {
            break;
        }
        unsetCursor();
        break;
    default:
        unsetCursor();
        break;
    }
}

double QVedWidget::currentZoom() const {
    return graphicEngine_.GetZoom();
}

void QVedWidget::setZoom(double zoom, const POINT* point) {
    if (point) {
        const double oldZoom = graphicEngine_.GetZoom();
        if (std::fabs(oldZoom) > 0.0) {
            zoomAtScreenPoint(QPoint(point->x, point->y), zoom / oldZoom);
            return;
        }
    }

    graphicEngine_.SetZoom(zoom);
    update();
}

bool QVedWidget::isRectVisible(TDMatRect rect) const {
    return graphicEngine_.IsRectVisible(rect);
}

long QVedWidget::mouseTolerance() const {
    return graphicEngine_.GetMouseTolerance();
}

double QVedWidget::minimumDistance() const {
    return graphicEngine_.GetMinimumDistance();
}

void QVedWidget::zoomInOutEvent(TDVOPZoomInOutExtVar* extVar) const {
    Q_UNUSED(extVar);
}

TDMatPoint QVedWidget::screenToRealPoint(const QPoint& screenPoint) const {
    return {
        graphicEngine_.ScreenToXPos(screenPoint.x()),
        graphicEngine_.ScreenToYPos(screenPoint.y())
    };
}

TDMatPoint QVedWidget::snapPointToGrid(TDMatPoint point) const {
    if (!gridLock_) {
        return point;
    }

    const double majorStep = documentSettings_ ? documentSettings_->gridSettings.majorStepReal : kDefaultGridMajorStep;
    const int subdivisions = documentSettings_ ? documentSettings_->gridSettings.subdivisions : kDefaultGridSubdivisions;
    const double snapStep = (subdivisions > 0) ? majorStep / static_cast<double>(subdivisions) : majorStep;
    point.x = std::round(point.x / snapStep) * snapStep;
    point.y = std::round(point.y / snapStep) * snapStep;
    return point;
}

TDOPVirtMouseButton QVedWidget::virtualButton(Qt::MouseButton button) const {
    switch (button) {
    case Qt::LeftButton:
        return VIRTMOUSEBTM_LEFT;
    case Qt::MiddleButton:
        return VIRTMOUSEBTM_MIDDLE;
    case Qt::RightButton:
        return VIRTMOUSEBTM_RIGHT;
    default:
        return VIRTMOUSEBTM_UNKNOWN;
    }
}

TDOPVirtMouseButton QVedWidget::virtualButtons(Qt::MouseButtons buttons) const {
    if (buttons.testFlag(Qt::LeftButton)) {
        return VIRTMOUSEBTM_LEFT;
    }
    if (buttons.testFlag(Qt::MiddleButton)) {
        return VIRTMOUSEBTM_MIDDLE;
    }
    if (buttons.testFlag(Qt::RightButton)) {
        return VIRTMOUSEBTM_RIGHT;
    }
    return VIRTMOUSEBTM_UNKNOWN;
}

TDOPVirtKeyState QVedWidget::virtualKeyState(Qt::KeyboardModifiers modifiers) const {
    TDOPVirtKeyState state = VKState_KEY_UNKNOWN;
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        state |= VKState_KEY_SHIFT;
    }
    if (modifiers.testFlag(Qt::ControlModifier)) {
        state |= VKState_KEY_CTRL;
    }
    if (modifiers.testFlag(Qt::AltModifier)) {
        state |= VKState_KEY_ALT;
    }
    return state;
}

TDMatPoint QVedWidget::operationPoint(const QPoint& screenPoint) const {
    return snapPointToGrid(screenToRealPoint(screenPoint));
}

void QVedWidget::updateSnapMousePoint(const QPoint& screenPoint) {
    if (!canvasRect().contains(screenPoint) ||
        (!shouldDrawMouseToleranceMarker() && !shouldDrawMouseToleranceCross())) {
        if (hasSnapMousePoint_) {
            hasSnapMousePoint_ = false;
            update();
        }
        return;
    }

    snapMousePoint_ = snapPointToGrid(screenToRealPoint(screenPoint));
    hasSnapMousePoint_ = true;
    update();
}

bool QVedWidget::shouldDrawMouseToleranceMarker() const {
    return activeCursor_ == VECVIEW_SELECT_OBJECT ||
           interactionMode_ == InteractionMode::SelectObject;
}

bool QVedWidget::shouldDrawMouseToleranceCross() const {
    if (!showMouseToleranceCross_) {
        return false;
    }

    return interactionMode_ == InteractionMode::CreateLine ||
           interactionMode_ == InteractionMode::CreateText ||
           interactionMode_ == InteractionMode::MoveObject ||
           interactionMode_ == InteractionMode::DeleteObject ||
           interactionMode_ == InteractionMode::ModifyObject;
}

void QVedWidget::notifyMouseCoordinate(const QPoint& screenPoint) {
    if (!mouseCoordinateCallback_) {
        return;
    }

    if (!canvasRect().contains(screenPoint)) {
        mouseCoordinateCallback_({0.0, 0.0}, false);
        return;
    }

    mouseCoordinateCallback_(operationPoint(screenPoint), true);
}

void QVedWidget::drawSnapMouseMarker() {
    if (!hasSnapMousePoint_) {
        return;
    }

    if (shouldDrawMouseToleranceMarker()) {
        graphicEngine_.DrawMouseToleranz(snapMousePoint_.x, snapMousePoint_.y);
        return;
    }

    if (shouldDrawMouseToleranceCross()) {
        graphicEngine_.DrawMouseToleranceCross(snapMousePoint_.x, snapMousePoint_.y);
    }
}

void QVedWidget::drawSelectionAreaMarker() {
    if (!selectionAreaActive_ || activeCursor_ == VECVIEW_DOCK) {
        return;
    }

    graphicEngine_.DrawBoxOutLine(selectionAreaStart_, selectionAreaCurrent_);
}
