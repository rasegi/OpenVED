#include "vec_graphic_engine_qt.h"

#include <QBitmap>
#include <QColor>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

QColor toQColor(TDRgbColor color) {
    return {
        static_cast<int>(color & 0xFF),
        static_cast<int>((color >> 8) & 0xFF),
        static_cast<int>((color >> 16) & 0xFF)
    };
}

QPen makePen(TDRgbColor color, bool outline) {
    QPen pen(toQColor(color));
    pen.setCosmetic(true);
    if (outline) {
        pen.setStyle(Qt::DotLine);
    }
    return pen;
}

long niceRulerStepAtLeast(double minimumStep) {
    if (minimumStep <= 1.0) {
        return 1;
    }

    const double exponent = std::floor(std::log10(minimumStep));
    const double base = std::pow(10.0, exponent);
    for (const double multiplier : {1.0, 2.0, 5.0, 10.0}) {
        const double candidate = multiplier * base;
        if (candidate >= minimumStep) {
            return static_cast<long>(std::ceil(candidate));
        }
    }

    return static_cast<long>(std::ceil(10.0 * base));
}

bool isMainRulerValue(long value, long mainStep) {
    return mainStep > 0 && value % mainStep == 0;
}

} // namespace

TDGraphicEngineQt::TDGraphicEngineQt()
    : painter_(nullptr),
      drawColor_(0),
      nodeSize_(3) {
    LoadDefaultNodeImages();
}

void TDGraphicEngineQt::SetPainter(QPainter* painter) {
    painter_ = painter;
}

void TDGraphicEngineQt::SetDeviceMetrics(long width, long height, double dpiX, double dpiY) {
    screenState_.SetViewSize(width, height);
    screenState_.SetDpi(static_cast<long>(dpiX), static_cast<long>(dpiY));
}

void TDGraphicEngineQt::LoadDefaultNodeImages() {
    nodeImages_.clear();

    const QPixmap strip(QStringLiteral(":/ved/nodes/nodes.bmp"));
    if (strip.isNull()) {
        return;
    }

    constexpr int nodeWidth = 8;
    constexpr int nodeHeight = 8;
    const int count = strip.width() / nodeWidth;
    nodeImages_.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        QPixmap node = strip.copy(index * nodeWidth, 0, nodeWidth, nodeHeight);
        node.setMask(node.createMaskFromColor(QColor(255, 0, 255), Qt::MaskInColor));
        nodeImages_.push_back(node);
    }
}

void TDGraphicEngineQt::SetNodeImages(std::vector<QPixmap> nodeImages) {
    nodeImages_ = std::move(nodeImages);
}

TDGraphicEngineScreenState& TDGraphicEngineQt::ScreenState() {
    return screenState_;
}

const TDGraphicEngineScreenState& TDGraphicEngineQt::ScreenState() const {
    return screenState_;
}

void TDGraphicEngineQt::DrawEllipse(const TDMatEllipse* pParams, bool bOutLine) {
    if (!painter_ || !pParams) {
        return;
    }

    TDMatEllipse tmpEllipse;
    tmpEllipse.xCenter = RealToXPos(MatRound(pParams->xCenter));
    tmpEllipse.yCenter = RealToYPos(MatRound(pParams->yCenter));
    tmpEllipse.xRadius = RealToXVal(MatRound(pParams->xRadius));
    tmpEllipse.yRadius = RealToYVal(MatRound(pParams->yRadius));
    tmpEllipse.nAngle = pParams->nAngle;

    TDPointArray ellipsePoints;
    ellipsePoints.reserve(1000);
    MatEllipseMidpoint(&tmpEllipse, &ellipsePoints);
    if (ellipsePoints.empty()) {
        return;
    }

    QPolygonF polygon;
    polygon.reserve(static_cast<int>(ellipsePoints.size()));
    for (const POINT& point : ellipsePoints) {
        polygon << QPointF(point.x, point.y);
    }

    painter_->save();
    if (bOutLine) {
        beginRasterNot();
    }
    painter_->setPen(bOutLine ? auxLinePen() : drawPen(false));
    painter_->setBrush(Qt::NoBrush);
    painter_->drawPolyline(polygon);
    painter_->restore();
}

void TDGraphicEngineQt::DrawLine(const TDMatLine* pParams, bool bOutLine) {
    if (!painter_ || !pParams) {
        return;
    }

    painter_->save();
    if (bOutLine) {
        beginRasterNot();
    }
    painter_->setPen(drawPen(bOutLine));
    painter_->drawLine(
        QPointF(RealToXPos(pParams->x1), RealToYPos(pParams->y1)),
        QPointF(RealToXPos(pParams->x2), RealToYPos(pParams->y2)));
    painter_->restore();
}

void TDGraphicEngineQt::DrawRectangle(const TDMatRect* pParams, bool bOutLine) {
    if (!painter_ || !pParams) {
        return;
    }

    painter_->save();
    if (bOutLine) {
        beginRasterNot();
    }
    painter_->setPen(drawPen(bOutLine));
    painter_->setBrush(Qt::NoBrush);
    QPolygonF polygon;
    polygon << QPointF(RealToXPos(pParams->P1.x), RealToYPos(pParams->P1.y))
            << QPointF(RealToXPos(pParams->P2.x), RealToYPos(pParams->P2.y))
            << QPointF(RealToXPos(pParams->P3.x), RealToYPos(pParams->P3.y))
            << QPointF(RealToXPos(pParams->P4.x), RealToYPos(pParams->P4.y));
    painter_->drawPolygon(polygon);
    painter_->restore();
}

void TDGraphicEngineQt::DrawPolygon(const TDMatPointsArray* pParams, bool bOutLine) {
    if (!painter_ || !pParams || pParams->empty()) {
        return;
    }

    QPolygonF polygon;
    polygon.reserve(static_cast<int>(pParams->size()));
    for (const TDMatPoint& point : *pParams) {
        polygon << QPointF(RealToXPos(point.x), RealToYPos(point.y));
    }

    painter_->save();
    if (bOutLine) {
        beginRasterNotXorPen();
    }
    painter_->setPen(drawPen(bOutLine));
    painter_->setBrush(Qt::NoBrush);
    painter_->drawPolyline(polygon);
    painter_->restore();
}

void TDGraphicEngineQt::DrawLineOutLine(const TDMatLine* pParams) {
    if (!painter_ || !pParams) {
        return;
    }

    painter_->save();
    beginRasterNot();
    painter_->setPen(drawPen(false));
    painter_->drawLine(
        QPointF(MatRound(pParams->x1), MatRound(pParams->y1)),
        QPointF(MatRound(pParams->x2), MatRound(pParams->y2)));
    painter_->restore();
}

void TDGraphicEngineQt::DrawConstructPolygon(const TDMatPointsArray* pParams) {
    if (!painter_ || !pParams || pParams->empty()) {
        return;
    }

    QPolygonF polygon;
    polygon.reserve(static_cast<int>(pParams->size()));
    for (const TDMatPoint& point : *pParams) {
        polygon << QPointF(RealToXPos(point.x), RealToYPos(point.y));
    }

    painter_->save();
    beginRasterNotXorPen();
    painter_->setPen(auxLinePen());
    painter_->setBrush(Qt::NoBrush);
    painter_->drawPolyline(polygon);
    painter_->restore();
}

void TDGraphicEngineQt::DrawBoxOutLine(TDMatPoint MatPoint1, TDMatPoint MatPoint2) {
    if (!painter_) {
        return;
    }

    const QRectF rect(
        QPointF(RealToXPos(MatRound(MatPoint1.x)), RealToYPos(MatRound(MatPoint1.y))),
        QPointF(RealToXPos(MatRound(MatPoint2.x)), RealToYPos(MatRound(MatPoint2.y))));

    painter_->save();
    beginRasterNot();
    painter_->setPen(auxLinePen());
    painter_->setBrush(Qt::NoBrush);
    painter_->drawRect(rect.normalized());
    painter_->restore();
}

void TDGraphicEngineQt::DrawRulers(long nDist, long nSubDiv, long nResLimit) {
    if (!painter_ || nDist <= 0 || nSubDiv <= 0) {
        return;
    }

    constexpr int kHorizontalRulerHeight = 24;
    constexpr int kVerticalRulerWidth = 34;
    constexpr int kMinorTick = 5;
    constexpr int kMajorTick = 10;
    constexpr int kLabelPadding = 3;
    constexpr int kMinimumLabelDistance = 42;

    const QRect clip = painter_->hasClipping()
        ? painter_->clipBoundingRect().toAlignedRect()
        : painter_->viewport();
    if (clip.width() <= 1 || clip.height() <= 1) {
        return;
    }

    const double minStepByPixel = std::max(
        std::fabs(ScreenToXVal(nResLimit)),
        std::fabs(ScreenToYVal(nResLimit)));
    const double minimumStep = std::max(
        static_cast<double>(std::max(1L, nDist / nSubDiv)),
        minStepByPixel);
    const long rulerStep = niceRulerStepAtLeast(minimumStep);
    const long mainStep = std::max(nDist, rulerStep);

    const double leftReal = std::min(ScreenToXPos(clip.left()), ScreenToXPos(clip.right()));
    const double rightReal = std::max(ScreenToXPos(clip.left()), ScreenToXPos(clip.right()));
    const double topReal = std::min(ScreenToYPos(clip.top()), ScreenToYPos(clip.bottom()));
    const double bottomReal = std::max(ScreenToYPos(clip.top()), ScreenToYPos(clip.bottom()));
    const long firstX = static_cast<long>(std::floor(leftReal / static_cast<double>(rulerStep))) * rulerStep;
    const long lastX = static_cast<long>(std::ceil(rightReal / static_cast<double>(rulerStep))) * rulerStep;
    const long firstY = static_cast<long>(std::floor(topReal / static_cast<double>(rulerStep))) * rulerStep;
    const long lastY = static_cast<long>(std::ceil(bottomReal / static_cast<double>(rulerStep))) * rulerStep;

    painter_->save();
    painter_->setClipping(false);

    const QRect horizontalRuler(clip.left(), clip.top(), clip.width(), kHorizontalRulerHeight);
    const QRect verticalRuler(clip.left(), clip.top(), kVerticalRulerWidth, clip.height());
    const QRect corner(clip.left(), clip.top(), kVerticalRulerWidth, kHorizontalRulerHeight);
    painter_->fillRect(horizontalRuler, QColor(238, 238, 234));
    painter_->fillRect(verticalRuler, QColor(238, 238, 234));
    painter_->fillRect(corner, QColor(228, 228, 224));

    painter_->setPen(QPen(QColor(150, 150, 145), 1));
    painter_->drawLine(horizontalRuler.bottomLeft(), horizontalRuler.bottomRight());
    painter_->drawLine(verticalRuler.topRight(), verticalRuler.bottomRight());

    painter_->setPen(QPen(QColor(35, 35, 35), 1));
    int lastHorizontalLabelRight = clip.left();
    for (long x = firstX; x <= lastX; x += rulerStep) {
        const int xScreen = RealToXPos(static_cast<double>(x));
        if (xScreen < clip.left() || xScreen > clip.right()) {
            continue;
        }

        const bool mainTick = isMainRulerValue(x, mainStep);
        const int tick = mainTick ? kMajorTick : kMinorTick;
        painter_->drawLine(xScreen, horizontalRuler.bottom(), xScreen, horizontalRuler.bottom() - tick);
        if (mainTick && xScreen - lastHorizontalLabelRight >= kMinimumLabelDistance) {
            const QString label = QString::number(x);
            painter_->drawText(
                xScreen + kLabelPadding,
                horizontalRuler.top() + painter_->fontMetrics().ascent() + 2,
                label);
            lastHorizontalLabelRight = xScreen + painter_->fontMetrics().horizontalAdvance(label) + kLabelPadding;
        }
    }

    int lastVerticalLabelBottom = clip.top();
    for (long y = firstY; y <= lastY; y += rulerStep) {
        const int yScreen = RealToYPos(static_cast<double>(y));
        if (yScreen < clip.top() || yScreen > clip.bottom()) {
            continue;
        }

        const bool mainTick = isMainRulerValue(y, mainStep);
        const int tick = mainTick ? kMajorTick : kMinorTick;
        painter_->drawLine(verticalRuler.right(), yScreen, verticalRuler.right() - tick, yScreen);
        if (mainTick && yScreen - lastVerticalLabelBottom >= kMinimumLabelDistance) {
            const QString label = QString::number(y);
            painter_->save();
            painter_->translate(verticalRuler.left() + painter_->fontMetrics().ascent() + 1, yScreen - kLabelPadding);
            painter_->rotate(-90);
            painter_->drawText(0, 0, label);
            painter_->restore();
            lastVerticalLabelBottom = yScreen + painter_->fontMetrics().horizontalAdvance(label) + kLabelPadding;
        }
    }

    painter_->restore();
}

void TDGraphicEngineQt::DrawNode(double x, double y, TDNodeType eNodeType, bool bLock) {
    if (!painter_ || eNodeType == NODE_UNKNOWN) {
        return;
    }

    const int index = static_cast<int>(eNodeType) + (bLock ? 1 : 0);
    if (index >= 0 && index < static_cast<int>(nodeImages_.size()) && !nodeImages_[index].isNull()) {
        painter_->drawPixmap(RealToXPos(x) - nodeSize_, RealToYPos(y) - nodeSize_, nodeImages_[index]);
        return;
    }

    const int halfSize = std::max(1, nodeSize_) / 2;
    QRectF rect(
        RealToXPos(x) - halfSize,
        RealToYPos(y) - halfSize,
        std::max(1, nodeSize_),
        std::max(1, nodeSize_));

    painter_->save();
    painter_->setPen(drawPen(bLock));
    painter_->setBrush(Qt::NoBrush);
    if (eNodeType == NODE_CONTROL || eNodeType == NODE_PROPORTIONAL) {
        painter_->drawEllipse(rect);
    } else {
        painter_->drawRect(rect);
    }
    painter_->restore();
}

void TDGraphicEngineQt::DrawFrame(const TDMatRect* pParams) {
    if (!painter_ || !pParams) {
        return;
    }

    QPolygonF polygon;
    polygon << QPointF(RealToXPos(MatRound(pParams->P1.x)), RealToYPos(MatRound(pParams->P1.y)))
            << QPointF(RealToXPos(MatRound(pParams->P2.x)), RealToYPos(MatRound(pParams->P2.y)))
            << QPointF(RealToXPos(MatRound(pParams->P3.x)), RealToYPos(MatRound(pParams->P3.y)))
            << QPointF(RealToXPos(MatRound(pParams->P4.x)), RealToYPos(MatRound(pParams->P4.y)))
            << QPointF(RealToXPos(MatRound(pParams->P1.x)), RealToYPos(MatRound(pParams->P1.y)));

    painter_->save();
    beginRasterNotXorPen();
    painter_->setPen(frameRectPen());
    painter_->setBrush(Qt::NoBrush);
    painter_->drawPolyline(polygon);
    painter_->restore();
}

long TDGraphicEngineQt::RealToXPos(double x) const {
    return screenState_.RealToXPos(x);
}

long TDGraphicEngineQt::RealToYPos(double y) const {
    return screenState_.RealToYPos(y);
}

long TDGraphicEngineQt::RealToXPos(long x) const {
    return screenState_.RealToXPos(x);
}

long TDGraphicEngineQt::RealToYPos(long y) const {
    return screenState_.RealToYPos(y);
}

long TDGraphicEngineQt::RealToXVal(double x) const {
    return screenState_.RealToXVal(x);
}

long TDGraphicEngineQt::RealToYVal(double y) const {
    return screenState_.RealToYVal(y);
}

long TDGraphicEngineQt::RealToXVal(long x) const {
    return screenState_.RealToXVal(x);
}

long TDGraphicEngineQt::RealToYVal(long y) const {
    return screenState_.RealToYVal(y);
}

double TDGraphicEngineQt::ScreenToXPos(long x) const {
    return screenState_.ScreenToXPos(x);
}

double TDGraphicEngineQt::ScreenToYPos(long y) const {
    return screenState_.ScreenToYPos(y);
}

double TDGraphicEngineQt::ScreenToXVal(long x) const {
    return screenState_.ScreenToXVal(x);
}

double TDGraphicEngineQt::ScreenToYVal(long y) const {
    return screenState_.ScreenToYVal(y);
}

void TDGraphicEngineQt::SetDrawColor(TDRgbColor nColor) {
    drawColor_ = nColor;
}

TDRgbColor TDGraphicEngineQt::GetDrawColor() const {
    return drawColor_;
}

int TDGraphicEngineQt::GetNodeSize() const {
    return nodeSize_;
}

void TDGraphicEngineQt::SetNodeSize(int nSize) {
    nodeSize_ = std::max(1, nSize);
}

bool TDGraphicEngineQt::IsRectVisible(TDMatRect MatRect) const {
    return screenState_.IsRectVisible(MatRect);
}

long TDGraphicEngineQt::GetMouseTolerance() const {
    return screenState_.GetMouseTolerance();
}

double TDGraphicEngineQt::GetMinimumDistance() const {
    return screenState_.GetMinimumDistance();
}

void TDGraphicEngineQt::SetPanX(long nPanX) {
    screenState_.SetPanX(nPanX);
}

void TDGraphicEngineQt::SetPanY(long nPanY) {
    screenState_.SetPanY(nPanY);
}

void TDGraphicEngineQt::SetPanAllowingWorldExpansion(long nPanX, long nPanY) {
    screenState_.SetPanAllowingWorldExpansion(nPanX, nPanY);
}

void TDGraphicEngineQt::ChangePanX(long xDiff) {
    screenState_.ChangePanX(xDiff);
}

void TDGraphicEngineQt::ChangePanY(long yDiff) {
    screenState_.ChangePanY(yDiff);
}

long TDGraphicEngineQt::GetPanX() const {
    return screenState_.GetPanX();
}

long TDGraphicEngineQt::GetPanY() const {
    return screenState_.GetPanY();
}

double TDGraphicEngineQt::GetMarginX() const {
    return screenState_.GetMarginX();
}

double TDGraphicEngineQt::GetMarginY() const {
    return screenState_.GetMarginY();
}

void TDGraphicEngineQt::SetMarginX(double x) {
    screenState_.SetMarginX(x);
}

void TDGraphicEngineQt::SetMarginY(double y) {
    screenState_.SetMarginY(y);
}

void TDGraphicEngineQt::SetMinimumDistance(long nPixels) {
    screenState_.SetMinimumDistance(nPixels);
}

void TDGraphicEngineQt::SetInchPerReal(double nNewInchPerReal) {
    screenState_.SetInchPerReal(nNewInchPerReal);
}

double TDGraphicEngineQt::GetInchPerReal() const {
    return screenState_.GetInchPerReal();
}

void TDGraphicEngineQt::SetWorldSpaceRange(const TDCoordinateSystem2D& newRange) {
    screenState_.SetWorldSpaceRange(newRange);
}

void TDGraphicEngineQt::SetWorldSpaceRange(double newLeft, double newTop, double newRight, double newBottom) {
    screenState_.SetWorldSpaceRange(newLeft, newTop, newRight, newBottom);
}

void TDGraphicEngineQt::SetWorkSpaceRange(const TDCoordinateSystem2D& newRange) {
    screenState_.SetWorkSpaceRange(newRange);
}

void TDGraphicEngineQt::SetWorkSpaceRange(double newLeft, double newTop, double newRight, double newBottom) {
    screenState_.SetWorkSpaceRange(newLeft, newTop, newRight, newBottom);
}

void TDGraphicEngineQt::SetViewRange(const TDCoordinateSystem2D& newRange) {
    screenState_.SetViewRange(newRange);
}

void TDGraphicEngineQt::SetViewRange(double newLeft, double newTop, double newRight, double newBottom) {
    screenState_.SetViewRange(newLeft, newTop, newRight, newBottom);
}

const TDCoordinateSystem2D& TDGraphicEngineQt::GetViewRange() const {
    return screenState_.GetViewRange();
}

const TDCoordinateSystem2D& TDGraphicEngineQt::GetWorldSpaceRange() const {
    return screenState_.GetWorldSpaceRange();
}

const TDCoordinateSystem2D& TDGraphicEngineQt::GetWorkSpaceRange() const {
    return screenState_.GetWorkSpaceRange();
}

double TDGraphicEngineQt::GetZoom() const {
    return screenState_.GetZoom();
}

void TDGraphicEngineQt::SetZoom(double nNewZoom) {
    screenState_.SetZoom(nNewZoom);
}

void TDGraphicEngineQt::SetZoomAtScreenPoint(double nNewZoom, long x, long y) {
    screenState_.SetZoomAtScreenPoint(nNewZoom, x, y);
}

void TDGraphicEngineQt::SetZoom(TDFraction newZoom) {
    screenState_.SetZoom(newZoom);
}

void TDGraphicEngineQt::SetZoomToRealSize() {
    screenState_.SetZoomToRealSize();
}

void TDGraphicEngineQt::SetViewSize(TDRecSize recSize) {
    screenState_.SetViewSize(recSize);
}

void TDGraphicEngineQt::SetViewSize(long width, long height) {
    screenState_.SetViewSize(width, height);
}

void TDGraphicEngineQt::DrawAuxLineX(long yPos) {
    if (!painter_) {
        return;
    }

    const QRect clip = painter_->clipBoundingRect().isEmpty()
        ? painter_->viewport()
        : painter_->clipBoundingRect().toAlignedRect();
    DrawAuxLine(clip.left(), RealToYPos(yPos), clip.right(), RealToYPos(yPos));
}

void TDGraphicEngineQt::DrawAuxLineY(long xPos) {
    if (!painter_) {
        return;
    }

    const QRect clip = painter_->clipBoundingRect().isEmpty()
        ? painter_->viewport()
        : painter_->clipBoundingRect().toAlignedRect();
    DrawAuxLine(RealToXPos(xPos), clip.top(), RealToXPos(xPos), clip.bottom());
}

void TDGraphicEngineQt::DrawAuxLine(long x1, long y1, long x2, long y2) {
    if (!painter_) {
        return;
    }

    painter_->save();
    painter_->setPen(auxLinePen());
    painter_->drawLine(QPointF(x1, y1), QPointF(x2, y2));
    painter_->restore();
}

void TDGraphicEngineQt::DrawGrid(long nWidth, long nHeight, long nDist, long nSubDiv, TDRgbColor nColor, long nResLimit) {
    if (!painter_) {
        return;
    }
    if (nWidth <= 0 || nHeight <= 0 || nDist < 1 || nSubDiv < 1) {
        return;
    }

    const long subDist = nDist / nSubDiv;
    if (subDist < 1) {
        return;
    }

    if (nResLimit > 0 && (RealToXVal(subDist) < nResLimit || RealToYVal(subDist) < nResLimit)) {
        return;
    }

    painter_->save();
    painter_->setPen(makePen(nColor, false));
    for (long y = nDist; y < nHeight; y += nDist) {
        for (long x = subDist; x < nWidth; x += subDist) {
            painter_->drawPoint(RealToXPos(x), RealToYPos(y));
        }
    }

    for (long x = nDist; x < nWidth; x += nDist) {
        for (long y = subDist; y < nHeight; y += subDist) {
            painter_->drawPoint(RealToXPos(x), RealToYPos(y));
        }
    }
    painter_->restore();
}

void TDGraphicEngineQt::DrawMouseToleranz(double x, double y) {
    if (!painter_) {
        return;
    }

    const long xScreen = RealToXPos(x);
    const long yScreen = RealToYPos(y);
    const long tolerance = GetMouseTolerance();

    painter_->save();
    beginRasterNot();
    painter_->setPen(drawPen(false));
    painter_->setBrush(Qt::NoBrush);
    painter_->drawRoundedRect(
        QRectF(
            xScreen - tolerance,
            yScreen - tolerance,
            tolerance * 2,
            tolerance * 2),
        tolerance,
        tolerance);
    painter_->restore();
}

void TDGraphicEngineQt::DrawMouseToleranceCross(double x, double y) {
    if (!painter_) {
        return;
    }

    const long xScreen = RealToXPos(x);
    const long yScreen = RealToYPos(y);
    const QRect clip = painter_->hasClipping()
        ? painter_->clipBoundingRect().toAlignedRect()
        : painter_->viewport();

    painter_->save();
    beginRasterNot();
    painter_->setPen(drawPen(false));
    painter_->drawLine(QPointF(clip.left(), yScreen), QPointF(clip.right(), yScreen));
    painter_->drawLine(QPointF(xScreen, clip.top()), QPointF(xScreen, clip.bottom()));
    painter_->restore();
}

QPen TDGraphicEngineQt::drawPen(bool outline) const {
    return makePen(drawColor_, outline);
}

QPen TDGraphicEngineQt::auxLinePen() const {
    return makePen(drawColor_, true);
}

QPen TDGraphicEngineQt::frameRectPen() const {
    QPen pen(QColor(191, 191, 191));
    pen.setCosmetic(true);
    pen.setWidth(3);
    return pen;
}

void TDGraphicEngineQt::beginRasterNot() {
    painter_->setBackgroundMode(Qt::TransparentMode);
    painter_->setCompositionMode(QPainter::RasterOp_NotDestination);
}

void TDGraphicEngineQt::beginRasterNotXorPen() {
    painter_->setBackgroundMode(Qt::TransparentMode);
    painter_->setCompositionMode(QPainter::RasterOp_NotSourceXorDestination);
}
